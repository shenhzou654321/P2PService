/*************************************************************************
    > File Name: deviceproxy.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年12月28日 星期一 17时01分39秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "config.h"
#include "deviceproxy.h"
#include "p2plibimpl.h"
#include "protocal.h"
#include "oshelper.h"

DeviceProxy::DeviceProxy() :
	m_pP2PAgent(NULL),
	m_userContext(NULL),
	m_sessionHandle(0),
	m_spMsgBuf(new CBuffer(MAX_PACKET_LENGTH + 2)),
	m_seq(0),
	m_relayState(RS_IDEL),
	m_retry(0),
	m_lastReqTick(0),
	m_spResendMsgBuf(new CBuffer(MAX_PACKET_LENGTH + 2)),
	m_peerDevId(0),
	m_pthread((pthread_t)-1),
	m_fNeedToExit(false),
	m_fThreadIsValid(false)
{

}

DeviceProxy::~DeviceProxy()
{

}

HRESULT DeviceProxy::Init(P2PAgentImpl* pP2PAgent)
{
	m_pP2PAgent = pP2PAgent;
	return S_OK; 
}

HRESULT DeviceProxy::Start()
{
	HRESULT hr = S_OK;
	int err = 0;

	ChkIf(m_fThreadIsValid, E_UNEXPECTED);
	err = ::pthread_create(&m_pthread, NULL, DeviceProxy::ThreadFunction, this);

	ChkIf(err != 0, ERRNO_TO_HRESULT(err));
	m_fThreadIsValid = true;

Cleanup:
	return hr;
}

void DeviceProxy::Stop()
{
	void* pRetValFromThread = NULL;
	m_fNeedToExit = true;

	if (m_fThreadIsValid)
	{
		::pthread_join(m_pthread, &pRetValFromThread);
	}
	m_fThreadIsValid = false;
	m_pthread = (pthread_t)-1;

	return;
}

void* DeviceProxy::ThreadFunction(void* pThis)
{
	((DeviceProxy*)pThis)->Run();
	return NULL;
}

void DeviceProxy::Run()
{
	while (!m_fNeedToExit)
	{
		// sleep 100ms
		usleep(100000);

		OnTimer();
	}
}

void DeviceProxy::HandlePeerData(void* buff, int size, const devid_t& fromDev)
{
	m_msgReader.Reset();
	m_msgReader.AddBytes((uint8_t*)buff, size);
	if (m_msgReader.GetState() != P2PMessageReader::BodyValidated)
	{
		return;
	}

	if (m_msgReader.CmdType() == P2P_STREAM)
	{
		// handle session data
		uint8_t* pdata = (uint8_t*)buff + TB_HEAD_SIZE;
		int offset = 0;
		uint32_t sessionHandle;
		uint8_t dataType;
		memcpy(&sessionHandle, pdata + offset, 4);
		sessionHandle = ntohl(sessionHandle);
		offset += 4; // SessionHandle
		dataType = pdata[offset];
		offset++;	// Data Type
		offset+= 3;	// Reserved

		uint8_t* pstream = pdata + offset;

		if (sessionHandle != m_sessionHandle)
		{
			std::cout << "HandlePeerData session handle invalid. Recved:" << sessionHandle << " Expected:" << m_sessionHandle << std::endl;
			return;
		}
		
		if (m_pP2PAgent != NULL)
		{
			m_pP2PAgent->HandleSessionData(pstream, size - TB_HEAD_SIZE - offset, m_userContext);
		}
		return;
	}

	string sCmd = m_msgReader.Cmd();
	if (sCmd.compare(ResponseCmd) == 0)
	{
		string sRetCmd = m_msgReader.RetCmd();
		if (sRetCmd.compare(P2PStartRelay) == 0)
		{
			HandleStartRelayResp(m_msgReader, fromDev);
			return;
		}
		if (sRetCmd.compare(P2PStopRelay) == 0)
		{
			HandleStopRelayResp(m_msgReader, fromDev);
			return;
		}

		return;
	}
	if (sCmd.compare(P2PNotifyStopRelay) == 0)
	{
		HandleRelayStopNotify(m_msgReader, fromDev);
		return;
	}
	if (sCmd.compare(P2PSessionMediaAttrNotify) == 0)
	{
		HandleSessionMediaAttrNotify(m_msgReader, fromDev);
		return;
	}
}

uint32_t DeviceProxy::StartSessionRelay(const char* sessionId, const char* url, void* userContext)
{
	if (m_pP2PAgent == NULL)
	{
		return P2P_SYSTEM_ERROR;
	}
	if (m_relayState == RS_Relaying 
			|| m_relayState == RS_WaitStartResp)
	{
		StopSessionRelay(m_sessionId.c_str());
	}

	m_sessionId = sessionId;
	m_url = url;
	m_userContext = userContext;
	m_sessionHandle = 0;
	m_relayState = RS_WaitStartResp;
	m_retry = 0;
	m_lastReqTick = GetMillisecondCounter();

	// build start session relay request
	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;

	protocalBuilder.Reset();
	protocalBuilder.AddRequestHead(P2PStartRelay,
			CMD_TYPE_NORMAL,
			true,
			S_THE_DEVID,
			"RELAY_NODE");
	protocalBuilder.AddDataPram(PK_sessionID, sessionId);
	protocalBuilder.AddDataPram(PK_url, url);
	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	uint32_t seqno = ++m_seq;
	HRESULT hr = msgBuilder.MakePacket(protocalBuilder.Commit(),
			seqno,
			PS_MSG_TYPE);
	if (FAILED(hr))
	{
		return P2P_SYSTEM_ERROR;
	}

	CRefCountedBuffer buff;
	msgBuilder.GetResult(&buff);
	
	if (buff->GetSize() < MAX_PACKET_LENGTH)
	{
		memcpy(m_spResendMsgBuf->GetData(), buff->GetData(), buff->GetSize());
		m_spResendMsgBuf->SetSize(buff->GetSize());
	}
	else
	{
		m_spResendMsgBuf->SetSize(0);
	}

	return m_pP2PAgent->Send2Peer(buff->GetData(), buff->GetSize(), m_peerDevId);
}

uint32_t DeviceProxy::StopSessionRelay(const char* sessionId)
{
	if (m_pP2PAgent == NULL)
	{
		return P2P_SYSTEM_ERROR;
	}
	if (m_sessionId.compare(sessionId) != 0)
	{
		return P2P_SESSION_ID_ERR;
	}

	m_sessionId = "";
	m_url = "";
	m_userContext = NULL;
	m_sessionHandle = 0;
	m_relayState = RS_WaitStopResp;
	m_retry = 0;
	m_lastReqTick = GetMillisecondCounter();

	// build start session relay request
	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;

	protocalBuilder.Reset();
	protocalBuilder.AddRequestHead(P2PStopRelay,
			CMD_TYPE_NORMAL,
			true,
			S_THE_DEVID,
			"RELAY_NODE");
	protocalBuilder.AddDataPram(PK_sessionID, sessionId);
	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	uint32_t seqno = ++m_seq;
	HRESULT hr = msgBuilder.MakePacket(protocalBuilder.Commit(),
			seqno,
			PS_MSG_TYPE);
	if (FAILED(hr))
	{
		return P2P_SYSTEM_ERROR;
	}

	CRefCountedBuffer buff;
	msgBuilder.GetResult(&buff);

	if (buff->GetSize() < MAX_PACKET_LENGTH)
	{
		memcpy(m_spResendMsgBuf->GetData(), buff->GetData(), buff->GetSize());
		m_spResendMsgBuf->SetSize(buff->GetSize());
	}
	else
	{
		m_spResendMsgBuf->SetSize(0);
	}

	return m_pP2PAgent->Send2Peer(buff->GetData(), buff->GetSize(), m_peerDevId);
}

void DeviceProxy::OnTimer()
{
	switch (m_relayState)
	{
		case RS_IDEL:
			break;
		case RS_WaitStartResp:
			if (GetMillisecondCounter() - m_lastReqTick > REQ_RESEND_INTERVEL)
			{
				if (m_retry > REQ_MAX_RESEND_COUNT)
				{
					// notify app
					if (m_pP2PAgent != NULL)
					{
						m_pP2PAgent->OnSessionEvent(se_start_relay_ng, m_sessionId.c_str(), P2P_RESP_TIMEOUT);
					}
					m_relayState = RS_IDEL;
				}
				else
				{
					m_retry++;
					m_lastReqTick = GetMillisecondCounter();
					if (m_pP2PAgent != NULL)
					{
						m_pP2PAgent->Send2Peer(m_spResendMsgBuf->GetData(), m_spResendMsgBuf->GetSize(), m_peerDevId);
					}
				}
			}
			break;
		case RS_WaitStopResp:
			if (GetMillisecondCounter() - m_lastReqTick > REQ_RESEND_INTERVEL)
			{
				if (m_retry > REQ_MAX_RESEND_COUNT)
				{
					if (m_pP2PAgent != NULL)
					{
						m_pP2PAgent->OnSessionEvent(se_stop_relay_ng, m_sessionId.c_str(), P2P_RESP_TIMEOUT);
					}
					m_relayState = RS_IDEL;
				}
				else
				{
					m_retry++;
					m_lastReqTick = GetMillisecondCounter();
					if (m_pP2PAgent != NULL)
					{
						m_pP2PAgent->Send2Peer(m_spResendMsgBuf->GetData(), m_spResendMsgBuf->GetSize(), m_peerDevId);
					}
				}
			}
			break;
		default:
			break;
	}
}

void DeviceProxy::HandleStartRelayResp(P2PMessageReader& msgReader, const devid_t& fromDev)
{
	const P2PRelayRespParse *parse = (const P2PRelayRespParse*)msgReader.GetNatProtocalParse();
	if (parse == NULL)
	{
		return;
	}
	if (msgReader.SeqNo() != m_seq)
	{
		return;
	}
	m_sessionHandle = parse->SessionHandle();
	m_relayState = RS_Relaying;
	if (m_pP2PAgent != NULL)
	{
		m_pP2PAgent->OnSessionEvent(se_start_relay_ok, m_sessionId.c_str(), P2P_OK);
	}
}

void DeviceProxy::HandleStopRelayResp(P2PMessageReader& msgReader, const devid_t& fromDev)
{
	const P2PRelayRespParse *parse = (const P2PRelayRespParse*)msgReader.GetNatProtocalParse();
	if (parse == NULL)
	{
		return;
	}
	if (msgReader.SeqNo() != m_seq)
	{
		return;
	}
	m_relayState = RS_IDEL;
	if (m_pP2PAgent != NULL)
	{
		m_pP2PAgent->OnSessionEvent(se_stop_relay_ok, m_sessionId.c_str(), P2P_OK);
	}
}

void DeviceProxy::HandleRelayStopNotify(P2PMessageReader& msgReader, const devid_t& fromDev)
{
	const P2PRelayReqParse *parse = (const P2PRelayReqParse*)msgReader.GetNatProtocalParse();
	if (parse == NULL)
	{
		return;
	}
	m_relayState = RS_IDEL;

	if (m_pP2PAgent == NULL)
	{
		return;
	}

	// build start session relay request
	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;

	protocalBuilder.Reset();
	protocalBuilder.AddResponseHead(P2PNotifyStopRelay,
			CMD_TYPE_NORMAL,
			S_THE_DEVID,
			"RELAY_NODE",
			ACK_OK,
			"OK");
	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	uint32_t seqno = msgReader.SeqNo();
	HRESULT hr = msgBuilder.MakePacket(protocalBuilder.Commit(),
			seqno,
			PS_MSG_TYPE);
	if (FAILED(hr))
	{
		return;
	}

	CRefCountedBuffer buff;
	msgBuilder.GetResult(&buff);

	m_pP2PAgent->Send2Peer(buff->GetData(), buff->GetSize(), m_peerDevId);

	m_pP2PAgent->OnSessionEvent(se_relay_stopped, m_sessionId.c_str(), P2P_OK);
}

void DeviceProxy::HandleSessionMediaAttrNotify(P2PMessageReader& msgReader, const devid_t& fromDev)
{
	const P2PRelayReqParse *parse = (const P2PRelayReqParse*)msgReader.GetNatProtocalParse();
	if (parse == NULL)
	{
		return;
	}
	m_relayState = RS_IDEL;

	string sessionId = parse->SessionId();
	uint32_t audioCodec = (uint32_t)(parse->AudioCodec());
	uint32_t audioSamplerate = (uint32_t)parse->AudioSamplerate();
	uint32_t audioChannel = (uint32_t)parse->AudioChannel();

	if (m_pP2PAgent == NULL)
	{
		return;
	}

	// build response packet
	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;

	protocalBuilder.Reset();
	protocalBuilder.AddResponseHead(P2PSessionMediaAttrNotify,
			CMD_TYPE_NORMAL,
			S_THE_DEVID,
			"RELAY_NODE",
			ACK_OK,
			"OK");
	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	uint32_t seqno = msgReader.SeqNo();
	HRESULT hr = msgBuilder.MakePacket(protocalBuilder.Commit(),
			seqno,
			PS_MSG_TYPE);
	if (FAILED(hr))
	{
		return;
	}

	CRefCountedBuffer buff;
	msgBuilder.GetResult(&buff);

	m_pP2PAgent->Send2Peer(buff->GetData(), buff->GetSize(), m_peerDevId);

	m_pP2PAgent->HandleSessionMediaAttr(m_sessionId.c_str(), 
			audioCodec,
			audioSamplerate,
			audioChannel);
}

void DeviceProxy::UpdatePeerDevId(devid_t devId)
{
	if (m_peerDevId != devId)
	{
		if (m_relayState == RS_Relaying)
		{
			m_pP2PAgent->OnSessionEvent(se_relay_stopped, m_sessionId.c_str(), P2P_DISCONNECTED);
		}
	}
	m_peerDevId = devId;	
}


