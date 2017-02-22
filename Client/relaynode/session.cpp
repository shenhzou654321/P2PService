/*************************************************************************
    > File Name: session.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2016年01月05日 星期二 18时36分58秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "session.h"
#include "devrepo.h"
#include "devengine.h"
#include "protocal.h"
#include "oshelper.h"
#include "devengine.h"
#include "messagebuilder.h"
#include <algorithm>
#include <iostream>
#include <sstream>

using namespace std;

static int PullerCallbackFunc(CBDataType dataType, void* data, void* obj);

const char SessionObjMagicWord[] = "My_Session_Magic_Word";

Session::Session(const char* sessionId,
		const char* url,
		uint32_t sessionHandle) :
	m_sessionId(sessionId),
	m_url(url),
	m_sessionHandle(sessionHandle),
	m_state(Init),
	m_spBuf(new CBuffer(MAX_PACKET_LENGTH)),
	m_spMsgBuf(new CBuffer(MAX_PACKET_LENGTH)),
	m_mediaAttrInited(false),
	m_handler(0),
	m_lastPacketTick(0),
	m_restartStreamTick(0),
	m_seqno(0),
	m_fStop(false),
	m_retryCount(0)
{
	assert(sessionId != NULL);
	assert(url != NULL);

	strcpy(m_magicWord, SessionObjMagicWord);
	std::cout << "Session Construct id:" << m_sessionId << ", sessionHandle:" << m_sessionHandle << endl;
}

Session::~Session()
{

}

void Session::AddDevice(devid_t devid, Device* pdev)
{
	dev_list_t::iterator iter;
	iter = find(m_devList.begin(), m_devList.end(), devid);
	if (iter == m_devList.end())
	{
		m_devList.push_back(devid);
	}

	if (m_mediaAttrInited)
	{
		// 通知终端会话流属性
		pdev->SendMediaAttr(m_sessionId.c_str(), &m_mediaAttr);
	}
}

void Session::RmDevice(devid_t devid)
{
	dev_list_t::iterator iter;
	iter = find(m_devList.begin(), m_devList.end(), devid);
	if (iter != m_devList.end())
	{
		m_devList.erase(iter);
	}
}

bool Session::StartStream()
{
	string sdevNo = "10001";
	char szPsw[] = "admin";
	int ret = 0;
	uint32_t devNo = DevEngine::Instance()->DevNo();
	ostringstream ssDevNo;
	ssDevNo << devNo;
	sdevNo = ssDevNo.str();

	std::cout << "Call RTSP_Puller_Create" << endl;
	m_handler = RTSP_Puller_Create();
	std::cout << "Call RTSP_Puller_SetCallback" << endl;
	ret = RTSP_Puller_SetCallback(m_handler, PullerCallbackFunc, this);
	std::cout << "Call RTSP_Puller_StartStream" << endl;
	ret = RTSP_Puller_StartStream(m_handler, 
			m_url.c_str(), 
			RTP_OVER_TCP, 
			sdevNo.c_str(),
			szPsw,
			0,	// reconnect
			1);	// rtp packet

	m_state = Receiving;

	std::cout << "Exit StartStream" << endl;

	return true;
}

void Session::StopStream()
{
	if (m_handler != 0)
	{
		RTSP_Puller_CloseStream(m_handler);
		RTSP_Puller_Release(m_handler);
		m_handler = 0;
		m_fStop = true;

		NotifySessionStop();
	}
}

void Session::RestartStream()
{
	m_restartStreamTick = GetMillisecondCounter();
}

void Session::DoRestartStream()
{
	m_retryCount++;
	m_restartStreamTick = 0;
	RTSP_Puller_CloseStream(m_handler);
	RTSP_Puller_Release(m_handler);
	m_handler = 0;
	StartStream();
}

void Session::ForwdData(void* buff, int size)
{
	//std::cout << "." << endl;
	m_lastPacketTick = GetMillisecondCounter();
	if (size + STREAM_HEAD_SIZE > (int)MAX_PACKET_LENGTH)
	{
		std::cout << "Stream packet size is too large " << size << endl;
		return;
	}
	uint8_t* pBuf = m_spBuf->GetData();
	uint32_t v = htonl(m_sessionHandle);
	memcpy(pBuf, &v, 4);		// Session Handle 4 bytes
	pBuf[4] = st_rtp;			// Type: rtp
	pBuf[5] = 0;				// Reserved 3 bytes
	pBuf[6] = 0;
	pBuf[7] = 0;

	memcpy((uint8_t*)m_spBuf->GetData() + STREAM_HEAD_SIZE, buff, size); 
	m_spBuf->SetSize(size + STREAM_HEAD_SIZE);

	CRefCountedBuffer spBuf;
	P2PMessageBuilder msgBuilder;
	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	HRESULT hr = msgBuilder.MakePacket(m_spBuf->GetData(),
			m_spBuf->GetSize(),
			m_seqno++,
			P2P_STREAM);
	if (FAILED(hr))
	{
		std::cout << "Session::ForwdData build msg failed" << endl;
		return;
	}
	msgBuilder.GetResult(&spBuf);

	dev_list_t::iterator iter;
	for (iter = m_devList.begin(); iter != m_devList.end(); iter++)
	{
		devid_t devid = *iter;
		DevEngine::Instance()->Send2Dev(spBuf->GetData(), spBuf->GetSize(), devid);
	}
}

void Session::NotifySessionStop()
{
	dev_list_t::iterator iter;
	for (iter = m_devList.begin(); iter != m_devList.end(); iter++)
	{
		devid_t devid = *iter;
		Device* pDev = DevRepo::Instance()->GetDevice(devid);
		if (pDev != NULL)
		{
			pDev->SendNotifyStopRelay(m_sessionId.c_str());
		}
	}
}

void Session::NotifyMediaAttr()
{
	dev_list_t::iterator iter;
	for (iter = m_devList.begin(); iter != m_devList.end(); iter++)
	{
		devid_t devid = *iter;
		Device* pDev = DevRepo::Instance()->GetDevice(devid);
		if (pDev != NULL)
		{
			pDev->SendMediaAttr(m_sessionId.c_str(), &m_mediaAttr);
		}
	}
}

bool Session::CheckMagicWord()const
{
	return strcmp(m_magicWord, SessionObjMagicWord) == 0;
}

void Session::InitMediaAttr(MediaAttr* mediaAttr)
{
	bool fNeedNotifyClient = false;
	if (memcmp(&m_mediaAttr, mediaAttr, sizeof(MediaAttr)) != 0)
	{
		fNeedNotifyClient = true;
	}
	m_mediaAttr = *mediaAttr;
	m_mediaAttrInited = true;
	if (fNeedNotifyClient)
	{
		NotifyMediaAttr();
	}
}

void Session::OnTimer()
{
	const uint32_t SessionTimeout = 30000;
	const uint32_t SessionRetryInterval = 100;
	if (m_lastPacketTick != 0)
	{
		if (GetMillisecondCounter() - m_lastPacketTick > SessionTimeout)
		{
			m_state = Timeout; 
			return;
		}
	}
	if (!m_fStop
		&& m_restartStreamTick != 0
		&& GetMillisecondCounter() - m_restartStreamTick > SessionRetryInterval)
	{
		if (m_retryCount >= PULLER_MAX_RETRY)
		{
			StopStream();
		}
		else
		{
			DoRestartStream();
		}
	}
}

int PullerCallbackFunc(CBDataType dataType, void* data, void* obj)
{
	Session* psess = (Session*)obj;
	if (!psess->CheckMagicWord())
	{
		std::cout << "PullerCallbackFunc obj value invalid: " << (int64_t)obj << endl;
		return -1;
	}

	if (dataType == CB_MEDIA_ATTR)
	{
		MediaAttr* mediaAttr = (MediaAttr*)data;
		std::cout << "PullerCallbackFunc - Media attr" << endl
			<< "Audio Codec:" << mediaAttr->audioCodec << endl
			<< "Audio Samplerate:" << mediaAttr->audioSamplerate << endl
			<< "Audio Channel:" << mediaAttr->audioChannel << endl;
		psess->InitMediaAttr(mediaAttr);
	}
	if (dataType == CB_RTP_DATA)
	{
		RTPData* rtpData = (RTPData*)data;
		psess->ForwdData(rtpData->dataBuf, rtpData->bufLen);
		uint8_t *pdata = (uint8_t*)rtpData->dataBuf;
		cout << "recv session data size:" << rtpData->bufLen << " first 8 bytes:"
		<< std::hex << (int)pdata[0] << " "
		<< std::hex << (int)pdata[1] << " "
		<< std::hex << (int)pdata[2] << " "
		<< std::hex << (int)pdata[3] << " "
		<< std::hex << (int)pdata[4] << " "
		<< std::hex << (int)pdata[5] << " "
		<< std::hex << (int)pdata[6] << " "
		<< std::hex << (int)pdata[7] << endl;
	}
	if (dataType == CB_PULLER_STATE)
	{
		PullerState* pullState = (PullerState*)data;
		std::cout << "Puller state " << pullState->resultCode << ":" << pullState->resultString << endl;

		if (pullState->resultCode != 0)
		{
			psess->RestartStream();
		}
	}

	return 0;
}


