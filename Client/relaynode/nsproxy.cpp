/*************************************************************************
    > File Name: nsproxy.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年12月31日 星期四 13时41分46秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "nsproxy.h"
#include "messagebuilder.h"
#include "protocalbuilder.h"
#include "oshelper.h"
#include "config.h"
#include "devengine.h"
#include "sessionrepo.h"
#include <sstream>
#include <iostream>

using namespace std;

const uint8_t DEV_MSG_TYPE = 0xA0;

NsProxy::NsProxy() : m_nsIpHostByteOrder(0), 
	m_nsPort(0),
	m_state(NsDefault), 
	m_heartbeatTick(0),
	m_seqNo(0), 
	m_spMsgBuf(new CBuffer(MAX_PACKET_LENGTH)),
	m_devId(""),
	m_regReqRetry(0),
	m_reqTickCount(0)
{

}

NsProxy::~NsProxy()
{

}

void NsProxy::Init(const char* nsIp, uint16_t nsPort, uint32_t devId, const char* devSn)
{
	assert(nsIp != NULL);
	assert(devSn != NULL);

	ostringstream ssdevId;
	ssdevId << (int)devId;

	m_nsIpHostByteOrder = ntohl(inet_addr(nsIp));
	m_nsPort = nsPort;
	m_devId = ssdevId.str();
	m_devSn = devSn;
	m_state = NsInit;
	m_regReqRetry = 0;
	m_reqTickCount = 0;

	ostringstream ss;
	ss << nsIp << ":" << nsPort;
	m_nsAddr = ss.str();
}

void NsProxy::OnTimer()
{
	const int REQ_RETRY_INTERVAL = 3000;	// 请求重发间隔
	int reqMaxRetry = Config::Instance()->ReqMaxRetry();
	int heartBeatInterval = Config::Instance()->HeartBeatInterval();
	int millisecondCounter = GetMillisecondCounter();
	switch (m_state)
	{
		case NsInit:
			SendRegisterReq();
			break;
		case NsWaitRegisterResp:
			if (millisecondCounter - m_reqTickCount > REQ_RETRY_INTERVAL)
			{
				if (m_regReqRetry > reqMaxRetry)
				{
					m_state = NsRegisterNG; 
					break;
				}
				m_regReqRetry++;
				SendRegisterReq();
			}
			break;
		case NsRegistered:
			if (millisecondCounter - m_reqTickCount > heartBeatInterval)
			{
				SendHeartbeatReq();
			}
			if (millisecondCounter - m_heartbeatTick > (uint32_t)(heartBeatInterval * reqMaxRetry))
			{
				std::cout << "NS heartbeat timeout " << m_heartbeatTick << endl;
				m_state = NsHeartBeatTimeout; 
			}
			break;
		default:
			break;
	}
}

void NsProxy::HandleNsData(void* buff, int size, uint32_t fromIpHostByteOrder, uint16_t fromPort)
{
	assert(buff != NULL);

	// check from address
	if (fromIpHostByteOrder != m_nsIpHostByteOrder
		|| fromPort != m_nsPort)
	{
		std::cout << "NSProxy::HandleNsData: check from address failed: " << fromIpHostByteOrder << ":" << fromPort
			<< "  " << m_nsIpHostByteOrder << ":" << m_nsPort << endl;
		return;
	}
/*
	std::cout << "NsProxy::HandleNsData: ";
	uint8_t* temp = (uint8_t*)buff;
	for (int i=0; i<size; i++)
	{
		printf("%02X ", temp[i]);
	}
	std::cout << endl;
*/
	m_msgReader.Reset();
	m_msgReader.AddBytes((uint8_t*)buff, size);
	if (m_msgReader.GetState() == P2PMessageReader::BodyValidated)
	{
//		std::cout << "Cmd: " << m_msgReader.Cmd() << endl;
		if (m_msgReader.BodyLength() == 0)
		{
			HandleHBResp(m_msgReader);
		}
		else if (strcmp(m_msgReader.Cmd(), ResponseCmd) == 0)
		{
//			std::cout << "RetCmd: " << m_msgReader.RetCmd() << endl;
			if (strcmp(m_msgReader.RetCmd(),"DeviceReg") == 0)
			{
				HandleRegResp(m_msgReader);
			}
		}
		else if (strcmp(m_msgReader.Cmd(), "StartRealPlay") == 0)
		{
			HandleStartRealPlay(m_msgReader);
		}
		else if (strcmp(m_msgReader.Cmd(), "StopRealPlay") == 0)
		{
			HandleStopRealPlay(m_msgReader);
		}
	}
	else
	{
		std::cout << "! P2PMessageReader::BodyValidated" << endl;
	}
}

void NsProxy::HandleRegResp(NSMessageReader& msgReader)
{
	if (msgReader.RetCode() == 200)
	{
		m_state = NsRegistered;
		m_heartbeatTick = GetMillisecondCounter();		// 设置心跳超时检查初始值
		std::cout << "NS Reg OK" << endl;
	}
	else
	{
		std::cout << "NS Reg fail: " << msgReader.RetCode() << endl;
		m_state = NsRegisterNG;
	}
}

void NsProxy::HandleHBResp(NSMessageReader& msgReader)
{
//	std::cout << "Recv NS HB resp" << endl;
	m_heartbeatTick = GetMillisecondCounter();
}

void NsProxy::HandleStartRealPlay(NSMessageReader& msgReader)
{
	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;
	CRefCountedBuffer spMsgBuf(new CBuffer(MAX_PACKET_LENGTH));
	protocalBuilder.Reset();
	const NSRealPlayReqParse* parse = (const NSRealPlayReqParse*)msgReader.GetNatProtocalParse();
	if (parse != NULL)
	{
		string sessionId = parse->SessionId();
		string url = parse->Url();
		// create session
		Session* sess = SessionRepo::Instance()->GetSession(sessionId);
		if (sess == NULL)
		{
			sess = SessionRepo::Instance()->CreateSession(sessionId, url);
		}
	
		protocalBuilder.AddResponseHead("StartRealPlay",
			CMD_TYPE_NORMAL,
			m_devId.c_str(),
			"NS",
			ACK_OK,
			"OK");
	}
	else
	{
		protocalBuilder.AddResponseHead("StartRealPlay",
			CMD_TYPE_NORMAL,
			m_devId.c_str(),
			"NS",
			500,
			"message parse failed");
	}
	msgBuilder.GetStream().Attach(spMsgBuf, true);
	uint32_t seqno = msgReader.SeqNo();
	HRESULT hr = msgBuilder.MakePacket(protocalBuilder.Commit(),
			seqno,
			DEV_MSG_TYPE);
	if (FAILED(hr))
	{
		return;
	}

	CRefCountedBuffer buff;
	msgBuilder.GetStream().GetBuffer(&buff);
	
	Send2NS(buff->GetData(), buff->GetSize());
}

void NsProxy::HandleStopRealPlay(NSMessageReader& msgReader)
{
	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;
	CRefCountedBuffer spMsgBuf(new CBuffer(MAX_PACKET_LENGTH));
	protocalBuilder.Reset();
	const NSRealPlayReqParse* parse = (const NSRealPlayReqParse*)msgReader.GetNatProtocalParse();
	if (parse != NULL)
	{
		string sessionId = parse->SessionId();
		SessionRepo::Instance()->DeleteSession(sessionId);
		protocalBuilder.AddResponseHead("StopRealPlay",
			CMD_TYPE_NORMAL,
			m_devId.c_str(),
			"NS",
			ACK_OK,
			"OK");
	}
	else
	{
		protocalBuilder.AddResponseHead("StopRealPlay",
			CMD_TYPE_NORMAL,
			m_devId.c_str(),
			"NS",
			500,
			"message parse failed");
	}
	msgBuilder.GetStream().Attach(spMsgBuf, true);
	uint32_t seqno = msgReader.SeqNo();
	HRESULT hr = msgBuilder.MakePacket(protocalBuilder.Commit(),
			seqno,
			DEV_MSG_TYPE);
	if (FAILED(hr))
	{
		return;
	}

	CRefCountedBuffer buff;
	msgBuilder.GetStream().GetBuffer(&buff);
	
	Send2NS(buff->GetData(), buff->GetSize());
}

bool NsProxy::SendRegisterReq()
{
	P2PMessageBuilder msgBuilder;
	CRefCountedBuffer spBuf;
	ProtocalBuilder protocalBuilder;
	protocalBuilder.Reset();
	protocalBuilder.AddRequestHead("DeviceReg",
			CMD_TYPE_NORMAL,
			true,
			m_devId.c_str(),
			"NS");
	protocalBuilder.AddDataPram("devSN", m_devSn.c_str());
	protocalBuilder.AddDataPram("support", "Transfer");
	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	msgBuilder.MakePacket(protocalBuilder.Commit(), ++m_seqNo, DEV_MSG_TYPE);
	msgBuilder.GetStream().GetBuffer(&spBuf);

	uint32_t ret = Send2NS(spBuf->GetData(), spBuf->GetSize()); 
	if (ret == P2P_OK)
	{
		m_state = NsWaitRegisterResp;
		m_reqTickCount = GetMillisecondCounter();
		return true;
	}

	m_state = NsRegisterNG;
	return false;
}

bool NsProxy::SendHeartbeatReq()
{
	P2PMessageBuilder msgBuilder;
	CRefCountedBuffer spBuf;
	ProtocalBuilder protocalBuilder;
	protocalBuilder.Reset();
	protocalBuilder.AddRequestHead("DeviceHB",
			CMD_TYPE_NORMAL,
			true,
			m_devId.c_str(),
			"NS");
	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	msgBuilder.MakePacket(NULL, 0, ++m_seqNo, NS_HEARTBEAT);
	msgBuilder.GetStream().GetBuffer(&spBuf);

	uint32_t ret = Send2NS(spBuf->GetData(), spBuf->GetSize()); 
	if (ret == P2P_OK)
	{
		m_reqTickCount = GetMillisecondCounter();
		return true;
	}

	return false;
}

uint32_t NsProxy::Send2NS(void* buff, int size)
{
/*	uint8_t* temp = (uint8_t*)buff;
	std::cout << "Send to NS:";
	for (int i=0; i<size; i++)
	{
		printf("%02X ", temp[i]);
	}
	std::cout << endl;*/
	return DevEngine::Instance()->Send2NS(buff, size);
}



