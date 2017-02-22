/*************************************************************************
    > File Name: device.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2016年01月04日 星期一 16时22分57秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "device.h"
#include "sessionrepo.h"
#include "session.h"
#include "messagebuilder.h"
#include "protocalbuilder.h"
#include "devengine.h"
#include <iostream>
#include <sstream>

using namespace std;

Device::Device(devid_t devid) : m_devid(devid),
	m_relaySessionId(""),
	m_relaySessionHandle(0),
	m_seqno(0),
	m_spMsgBuf(new CBuffer(MAX_PACKET_LENGTH))
{
	ostringstream ss;
	ss << (int)devid;
	m_sDevId = ss.str();
}

Device::~Device()
{

}

void Device::HandlePeerData(void* buff, int size)
{
	assert(buff != NULL);

	m_msgReader.Reset();
	m_msgReader.AddBytes((uint8_t*)buff, size);
	if (m_msgReader.GetState() == P2PMessageReader::BodyValidated)
	{
		string cmd = m_msgReader.Cmd();
		if (cmd == P2PStartRelay)
		{
			HandleStartRelayReq(m_msgReader);
		}
		else if (cmd == P2PStopRelay)
		{
			HandleStopRelayReq(m_msgReader);
		}
		else if (cmd == ResponseCmd)
		{
			string ackCmd = m_msgReader.RetCmd();
			if (ackCmd == P2PNotifyStopRelay)
			{
				HandleNotifyStopRelayResp(m_msgReader);
			}
		}
	}
	else
	{
		std::cout << "! P2PMessageReader::BodyValidated" << endl;
	}
}

void Device::HandleStartRelayReq(P2PMessageReader& msgReader)
{
	const P2PRelayReqParse *parse = (const P2PRelayReqParse*)msgReader.GetNatProtocalParse();
	if (parse == NULL)
	{
		return;
	}
	string sessionId = parse->SessionId();
	string url = parse->Url();
	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;
	protocalBuilder.Reset();
	uint32_t sessionHandle = 0;

	// add device to session
	Session* sess = SessionRepo::Instance()->GetSession(sessionId);
	if (sess == NULL)
	{
		sess = SessionRepo::Instance()->CreateSession(sessionId, url);
	}
	if (sess != NULL)
	{
		sess->AddDevice(m_devid, this);
		sessionHandle = sess->Handle();
		protocalBuilder.AddResponseHead(P2PStartRelay,
			CMD_TYPE_NORMAL,
			"RELAY_NODE",
			m_sDevId.c_str(),
			ACK_OK,
			"OK");
	}
	else
	{
		protocalBuilder.AddResponseHead(P2PStartRelay,
			CMD_TYPE_NORMAL,
			"RELAY_NODE",
			m_sDevId.c_str(),
			ACK_PULL_STREAM_FAIL,
			"Pull session stream failed");
	}

	std::cout << "SessionHandle is " << sessionHandle << endl;
	protocalBuilder.AddDataPram(PK_sessionHandle, (int)sessionHandle);
	uint32_t seqno = msgReader.SeqNo();
	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	HRESULT hr = msgBuilder.MakePacket(protocalBuilder.Commit(),
			seqno,
			PS_MSG_TYPE);
	if (FAILED(hr))
	{
		return;
	}

	CRefCountedBuffer spBuf;
	msgBuilder.GetResult(&spBuf);

	// send response to device
	DevEngine::Instance()->Send2Dev(spBuf->GetData(), spBuf->GetSize(), m_devid);
}

void Device::HandleStopRelayReq(P2PMessageReader& msgReader)
{
	const P2PRelayReqParse *parse = (const P2PRelayReqParse*)msgReader.GetNatProtocalParse();
	if (parse == NULL)
	{
		return;
	}
	string sessionId = parse->SessionId();
	int sessionHandle = 0;

	// remove device to session
	Session* sess = SessionRepo::Instance()->GetSession(sessionId);
	if (sess != NULL)
	{
		sess->RmDevice(m_devid);
	}

	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;
	protocalBuilder.Reset();
	protocalBuilder.AddResponseHead(P2PStopRelay,
			CMD_TYPE_NORMAL,
			"RELAY_NODE",
			m_sDevId.c_str(),
			ACK_OK,
			"OK");
	protocalBuilder.AddDataPram(PK_sessionHandle, sessionHandle);
	uint32_t seqno = msgReader.SeqNo();
	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	HRESULT hr = msgBuilder.MakePacket(protocalBuilder.Commit(),
			seqno,
			PS_MSG_TYPE);
	if (FAILED(hr))
	{
		return;
	}

	CRefCountedBuffer spBuf;
	msgBuilder.GetResult(&spBuf);

	// send response to device
	DevEngine::Instance()->Send2Dev(spBuf->GetData(), spBuf->GetSize(), m_devid);
}

void Device::HandleNotifyStopRelayResp(P2PMessageReader& msgReader)
{
	uint32_t seqno = 0;
	seqno = msgReader.SeqNo();
	m_resend.CheckResp(seqno);
}

bool Device::SendNotifyStopRelay(const char* sessionId)
{
	P2PMessageBuilder msgBuilder;
	CRefCountedBuffer spBuf;
	ProtocalBuilder protocalBuilder;
	protocalBuilder.Reset();
	protocalBuilder.AddRequestHead(P2PNotifyStopRelay,
			CMD_TYPE_NORMAL,
			true,
			"RELAY_NODE",
			m_sDevId.c_str());
	protocalBuilder.AddDataPram(PK_sessionID, sessionId);
	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	uint32_t seqno = ++m_seqno;
	HRESULT hr = msgBuilder.MakePacket(protocalBuilder.Commit(),
			seqno,
			PS_MSG_TYPE);
	if (FAILED(hr))
	{
		return false;
	}
	msgBuilder.GetResult(&spBuf);
	
	// Send to device
	DevEngine::Instance()->Send2Dev(spBuf->GetData(), spBuf->GetSize(), m_devid);

	m_resend.SetData(spBuf, seqno);

	return true;
}

bool Device::SendMediaAttr(const char* sessionId, MediaAttr* pmediaAttr)
{
	P2PMessageBuilder msgBuilder;
	CRefCountedBuffer spBuf;
	ProtocalBuilder protocalBuilder;
	protocalBuilder.Reset();
	protocalBuilder.AddRequestHead(P2PSessionMediaAttrNotify,
			CMD_TYPE_NORMAL,
			true,
			"RELAY_NODE",
			m_sDevId.c_str());
	protocalBuilder.AddDataPram(PK_sessionID, sessionId);
	protocalBuilder.AddDataPram(PK_audioCodec, pmediaAttr->audioCodec);
	protocalBuilder.AddDataPram(PK_audioSamplerate, pmediaAttr->audioSamplerate);
	protocalBuilder.AddDataPram(PK_audioChannel, pmediaAttr->audioChannel);
	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	uint32_t seqno = ++m_seqno;
	HRESULT hr = msgBuilder.MakePacket(protocalBuilder.Commit(),
			seqno,
			PS_MSG_TYPE);
	if (FAILED(hr))
	{
		std::cout << "Make Media Attr packet failed" << endl;
		return false;
	}
	msgBuilder.GetResult(&spBuf);
	
	// Send to device
	DevEngine::Instance()->Send2Dev(spBuf->GetData(), spBuf->GetSize(), m_devid);

	m_resend.SetData(spBuf, seqno);

	return true;
}

void Device::OnTimer()
{
	CRefCountedBuffer spBuf;
	if (m_resend.GetData(spBuf))
	{
		DevEngine::Instance()->Send2Dev(spBuf->GetData(), spBuf->GetSize(), m_devid);
	}
}


