/*************************************************************************
    > File Name: p2cconn.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月25日 星期三 16时23分30秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "p2pconn.h"
#include "config.h"
#include "protocal.h"
#include "psagent.h"
#include "p2plibimpl.h"
#include "p2psocketthread.h"
#include "resolvehostname.h"
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

P2PConn::P2PConn() :
	m_state(InitState),
	m_seq(0),
	m_pPSAgent(NULL),
	m_pP2PAgent(NULL),
	m_targetDevid(0),
	m_stateTick(0),
	m_hbRecvTick(0),
	m_hbSendTick(0),
	m_fMaster(false),
	m_fReportTestResult(false),
	m_spMsgBuf(new CBuffer(MAX_PACKET_LENGTH + 2))
{
	m_peerAddr = CSocketAddress(0xaaaaaaaa, 0);
	m_nsAddr = CSocketAddress(0xbbbbbbbb, 0);
	m_stunServerAddr = CSocketAddress(0xcccccccc, 0);
	memset(m_traversalId.id, 0, sizeof(m_traversalId.id));
	cout << "Create P2PConn " << m_traversalId.id << endl;
}

P2PConn::~P2PConn()
{
	Clear();
	cout << "Destory P2PConn " << m_traversalId.id << endl;
}

HRESULT P2PConn::Init(const traversal_id_t& traversalId,
			PSAgent* pPSAgent,
			P2PAgentImpl* pP2PAgent)
{
	HRESULT hr = S_OK;
	CSocketAddress addrLocal = CSocketAddress(0, 0);
	P2PSocketThread* pP2PSocketThread = NULL;

	ChkIfA(pPSAgent == NULL, E_FAIL);
	ChkIfA(pP2PAgent == NULL, E_FAIL);

	m_traversalId = traversalId;
	m_pPSAgent = pPSAgent;
	m_pP2PAgent = pP2PAgent;
	pP2PSocketThread = m_pP2PAgent->GetP2PSocketThread();
	ChkIfA(pP2PSocketThread == NULL, E_FAIL);

	hr = m_sock.UDPInit(addrLocal, this);
	if (FAILED(hr))
	{
		printf("UDP init failed: %08x\n", (int)hr);
		P2P_SetLastError(P2P_SOCKET_NG);
		Chk(hr);
	}
	m_sock.EnablePktInfoOption(true);

	m_stunProcess.Init(m_sock.GetSocketHandle());

	// 注册到数据接收线程
	pP2PSocketThread->AddP2PSocket(&m_sock);

Cleanup:
	return hr;
}

void P2PConn::Clear()
{
	P2PSocketThread* pP2PSocketThread = NULL;
	if (m_pP2PAgent != NULL)
	{
		pP2PSocketThread = m_pP2PAgent->GetP2PSocketThread();
		if (pP2PSocketThread != NULL)
		{
			pP2PSocketThread->RemoveP2PSocket(&m_sock);
		}
	}
	m_sock.Close();
}

void P2PConn::HandleMsg(CRefCountedBuffer &spBufferIn,
		CSocketAddress &fromAddr,
		CSocketAddress &localAddr,
		int sock)
{
	uint16_t stag = 0;
	if (spBufferIn->GetSize() <= 2)
	{
		return;
	}

	memcpy(&stag, spBufferIn->GetData(), 2);
	if (stag == PEER_DATA_STAG)
	{
		// upper lever message
		if (m_pP2PAgent != NULL)
		{
			m_pP2PAgent->HandlePeerData(spBufferIn->GetData() + 2, 
					spBufferIn->GetSize() - 2,
					m_targetDevid);
		}
	}
	else if (stag == PEER_CMD_STAG)
	{
		// traversal message
		m_msgReader.Reset();
		m_msgReader.AddBytes(spBufferIn->GetData() + 2,
				spBufferIn->GetSize() -2);
		if (m_msgReader.GetState() == P2PMessageReader::BodyValidated)
		{
			HandleTraversalMsg(m_msgReader, fromAddr);
		}
	}
	else
	{
		// stun message
		std::cout << "Handle stun message" << endl;
		m_stunProcess.HandleMsg(spBufferIn, fromAddr, localAddr, sock);
	}
}

void P2PConn::HandleTraversalMsg(P2PMessageReader& msgReader,
		CSocketAddress& fromAddr)
{
	if (msgReader.GetState() != P2PMessageReader::BodyValidated)
	{
		std::cout << "P2PConn::HandleTraversalMsg msgReader state error" << endl;
		return;
	}
	string sSubCmd = msgReader.SubCmd();

	if (sSubCmd.compare(ResponseCmd) == 0)
	{
		HandleAck(msgReader, fromAddr);
	}
	else
	{
		HandleReq(msgReader, fromAddr);
	}
}

void P2PConn::OnTick()
{
	if (m_state == InitState)
	{
		return;
	}

	if (m_state == WaitBindingTestResult)
	{
		m_stunProcess.OnTick();
	}

	if (m_fMaster)
	{
		MasterOnTick();
	}
	else
	{
		SlaveOnTick();
	}
	
	CheckResendRequest();

	TimeoutCheck();
}

void P2PConn::TimeoutCheck()
{
	if (m_state == InitState
			|| m_state == Timeout
			|| m_state == Error
			|| m_state == Stopped
			|| m_state == Disconnected)
	{
		return;
	}

	if (m_state == Connected)
	{
		// 心跳检查
		if (m_hbRecvTick != 0
			&& GetMillisecondCounter() - m_hbRecvTick > 3 * PEER_HB_INTERVAL)
		{
			UpdateState(Disconnected);
						
			// need send nodes traversal retry
			SendNodesTraversalRetry();
		}
	}
	else
	{
		// 超时检查
		if (m_stateTick != 0
			&& GetMillisecondCounter() - m_stateTick > STATE_TIMEOUT)
		{
			std::cout << "Traversal timeout current state:" << (int)m_state << endl;
			UpdateState(Timeout);

			// need report traversal failed
			SendTraversalResult(m_traversalId, ACK_TRAVERSAL_TIMEOUT);
		}
	}
}

void P2PConn::MasterOnTick()
{
	switch (m_state)
	{
		case WaitBindingTestResult:
			OnMasterWaitBindingTestResult();
			break;
		case WaitTraversalResponse:
			OnMasterWaitTraversalResponse();
			break;
		case WaitConnAck:
			OnMasterWaitConnAck();
			break;
		case Connected:
			OnMasterConnected();
			break;
		default:
			break;
	}
}

void P2PConn::SlaveOnTick()
{
	switch (m_state)
	{
		case WaitBindingTestResult:
			OnSlaveWaitBindingTestResult();
			break;
		case WaitConnTry2:
			OnSlaveWaitConnTry2();
			break;
		case Connected:
			OnSlaveConnected();
			break;
		default:
			break;
	}
}

void P2PConn::OnMasterWaitBindingTestResult()
{
	HRESULT hr = S_OK;
	uint32_t seqno;
	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;
	if (m_stunProcess.State() == StunProcess::Completed)
	{
		std::cout << "STUN binding test completed" << endl;
		// 发送打通请求
		protocalBuilder.Reset();
		protocalBuilder.AddRequestHead(PASSTHROUGH_CMD,
			CMD_TYPE_NORMAL,
			true,
			S_THE_DEVID,
			PS_SOURCE);
		protocalBuilder.AddDataPram(PK_subCmd, NATTraversalReq);
		protocalBuilder.AddDataPram(PK_traversalID, m_traversalId.id);
		protocalBuilder.AddDataPram(PK_devID, THE_DEVID);
		protocalBuilder.AddDataPram(PK_NATInfo1, m_stunProcess.GetMappedAddrInfo().mappedAddrPP);
		protocalBuilder.AddDataPram(PK_NATInfo2, m_stunProcess.GetMappedAddrInfo().mappedAddrAA);
		protocalBuilder.AddDataPram(PK_localIPInfo, m_stunProcess.GetMappedAddrInfo().localAddr);
		protocalBuilder.AddDataPram(PK_STUNSvrInfo, m_stunServerAddr);
		msgBuilder.GetStream().Attach(m_spMsgBuf, true);
		seqno = m_seq++;
		hr = msgBuilder.MakePacket(protocalBuilder.Commit(),
				seqno,
				PS_MSG_TYPE);
		if (FAILED(hr))
		{
			std::cout << "OnMasterWaitBindingTestResult msgBilder::MakePacket failed" << endl;
			UpdateState(Error);
			return;
		}
		hr = SendToPS(msgBuilder);
		if (FAILED(hr))
		{
			std::cout << "OnMasterWaitBindingTestResult SendToPS failed" << endl;
			UpdateState(Error);
			return;
		}
		else
		{
			UpdateState(WaitTraversalResponse);
		}

		// 更新重传请求
		CRefCountedBuffer buff;
		msgBuilder.GetResult(&buff);
		UpdateResendRequest(m_nsAddr, buff,	NATTraversalReq, true);
	}
}

void P2PConn::OnMasterWaitTraversalResponse()
{
	
}

void P2PConn::OnMasterWaitConnAck()
{

}

void P2PConn::OnMasterConnected()
{
	CheckAndSendHB();
}

void P2PConn::OnSlaveWaitBindingTestResult()
{
	HRESULT hr = S_OK;
	uint32_t seqno;
	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;
	if (m_stunProcess.State() == StunProcess::Completed)
	{
		std::cout << "OnSlaveWaitBindingTestResult STUN binding test completed" << endl;
		// 发送打通尝试
		ForcastAndSendConnTry();

		// 发送穿透应答
		protocalBuilder.Reset();
		protocalBuilder.AddRequestHead(PASSTHROUGH_CMD,
			CMD_TYPE_NORMAL,
			true,
			S_THE_DEVID,
			PS_SOURCE);
		protocalBuilder.AddDataPram(PK_subCmd, NATTraversalResp);
		protocalBuilder.AddDataPram(PK_devID, m_targetDevid);
		protocalBuilder.AddDataPram(PK_traversalID, m_traversalId.id);
		protocalBuilder.AddDataPram(PK_NATInfo1, m_stunProcess.GetMappedAddrInfo().mappedAddrPP);
		protocalBuilder.AddDataPram(PK_NATInfo2, m_stunProcess.GetMappedAddrInfo().mappedAddrAA);
		protocalBuilder.AddDataPram(PK_localIPInfo, m_stunProcess.GetMappedAddrInfo().localAddr);
		msgBuilder.GetStream().Attach(m_spMsgBuf, true);
		seqno = m_seq++;
		hr = msgBuilder.MakePacket(protocalBuilder.Commit(),
				seqno,
				PS_MSG_TYPE);
		if (FAILED(hr))
		{
			std::cout << "OnSlaveWaitBindingTestResult msgBuilde.MakePacket failed" << endl;
			UpdateState(Error);
			return;
		}
		hr = SendToPS(msgBuilder);
		if (FAILED(hr))
		{
			std::cout << "OnSlaveWaitBindingTestResult failed" << endl;
			UpdateState(Error);
			return;
		}
		else
		{
			UpdateState(WaitConnTry2);
		}

		// 更新重传请求
		CRefCountedBuffer buff;
		msgBuilder.GetResult(&buff);
		UpdateResendRequest(m_nsAddr, buff,	NATTraversalResp, true);
	}
}

void P2PConn::OnSlaveWaitConnTry2()
{

}

void P2PConn::OnSlaveConnected()
{
	CheckAndSendHB();
}

void P2PConn::CheckAndSendHB()
{
	HRESULT hr = S_OK;
	uint32_t seqno = 0;
	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;
	if (GetMillisecondCounter() - m_hbSendTick > PEER_HB_INTERVAL)
	{
		protocalBuilder.Reset();
		protocalBuilder.AddRequestHead(PASSTHROUGH_CMD,
			CMD_TYPE_NORMAL,
			true,
			THE_DEVID,
			m_targetDevid);
		protocalBuilder.AddDataPram(PK_subCmd, NATTraversalHB);
		protocalBuilder.AddDataPram(PK_traversalID, m_traversalId.id);
		msgBuilder.GetStream().Attach(m_spMsgBuf, true);	
		seqno = m_seq++;
		hr = msgBuilder.MakePacket(protocalBuilder.Commit(),
				seqno,
				PS_MSG_TYPE);
		if (FAILED(hr))
		{
			std::cout << "P2PConn::CheckAndSendHB MakePacket failed" << endl;
			return;
		}

		CRefCountedBuffer buff;
		if (FAILED(msgBuilder.GetResult(&buff)))
		{
			std::cout << "P2PConn::CheckAdnSendHB MsgBuilder get result failed" << endl;
			return;
		}

		Send2Peer(buff->GetData(), buff->GetSize(), false);
		m_hbSendTick = GetMillisecondCounter();
	}
}

uint32_t P2PConn::Send2Peer(const void* buff, int size, bool fIsData)
{
	uint32_t ret = P2P_OK;
	int sendret = -1;
	int sendsock = -1;
	uint16_t stag;
	CRefCountedBuffer sendBuff = CRefCountedBuffer(new CBuffer());

	if (size > (int)MAX_PACKET_LENGTH || size <= 0)
	{
		std::cout << "P2PConn::Send2Peer invalid size:" << size << endl;
		return P2P_DATA_PACKET_SIZE_NG; 
	}

	if (!IsConnected())
	{
//		std::cout << "P2PConn::Send2Peer peer not connected" << endl;
//		std::cout << "Send: " << (char*)buff + TB_HEAD_SIZE;
		return P2P_NOT_CONNECTED;
	}
	
	sendsock = m_sock.GetSocketHandle();
	if (sendsock == -1)
	{
		std::cout << "P2PConn::Send2Peer sendsock error:" << (int)sendsock << endl;
		return P2P_SEND_DATA_NG; 
	}

	sendBuff->InitWithAllocation(size + 2);
	if (fIsData)
	{
		stag = PEER_DATA_STAG;
	}
	else
	{
		stag = PEER_CMD_STAG;
	}
	stag = htons(stag);
	memcpy(sendBuff->GetData(), &stag, 2);
	memcpy(sendBuff->GetData() + 2, buff, size);

	sendret = ::sendto(sendsock, 
			sendBuff->GetData(), 
			size + 2, 
			0, 
			m_peerAddr.GetSockAddr(), 
			m_peerAddr.GetSockAddrLength());

	if (sendret != size + 2)
	{
		std::cout << "P2PConn::Send2Peer send data failed";
		ret = P2P_SEND_DATA_NG;
		P2P_SetLastError(ret);
	}

	return ret;
}

uint32_t P2PConn::SendCmd(const void* buff, int size, CSocketAddress& addr)
{
	uint32_t ret = P2P_OK;
	int sendret = -1;
	int sendsock = -1;
	uint16_t stag = PEER_CMD_STAG; 
	CRefCountedBuffer sendBuff = CRefCountedBuffer(new CBuffer());

	if (size > (int)MAX_PACKET_LENGTH || size <= 0)
	{
		std::cout << "P2PConn::SendCmd invalid size:" << size << endl;
		return P2P_DATA_PACKET_SIZE_NG; 
	}
	
	sendsock = m_sock.GetSocketHandle();
	if (sendsock == -1)
	{
		std::cout << "P2PConn::SendCmd sendsock error:" << (int)sendsock << endl;
		return P2P_SEND_DATA_NG; 
	}

	sendBuff->InitWithAllocation(size + 2);
	stag = htons(stag);
	memcpy(sendBuff->GetData(), &stag, 2);
	memcpy(sendBuff->GetData() + 2, buff, size);

	sendret = ::sendto(sendsock, 
			sendBuff->GetData(), 
			size + 2, 
			0, 
			addr.GetSockAddr(), 
			addr.GetSockAddrLength());
	std::cout << ">";
	if (sendret != size + 2)
	{
		std::cout << "P2PConn::SendCmd send data failed" << endl;
		ret = P2P_SEND_DATA_NG;
		P2P_SetLastError(ret);
	}

	return ret;
}

void P2PConn::SendNodesTraversalRetry()
{
	std::cout << "P2PConn::SendNodesTraversalRetry()" << endl;
	uint32_t seqno;
	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;
	protocalBuilder.Reset();
	protocalBuilder.AddRequestHead(PASSTHROUGH_CMD,
			CMD_TYPE_NORMAL,
			true,
			S_THE_DEVID,
			PS_SOURCE);
	protocalBuilder.AddDataPram("subCmd", NodesTraversalRetry);
	protocalBuilder.AddDataPram("traversalID", m_traversalId.id);

	seqno = m_seq++;
	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	msgBuilder.MakePacket(
			protocalBuilder.Commit(),
			seqno,
		PS_MSG_TYPE);

	SendToPS(msgBuilder);

	// 更新重传请求
	CRefCountedBuffer buff;
	msgBuilder.GetResult(&buff);
	UpdateResendRequest(m_nsAddr, buff,	NodesTraversalRetry, true);
}

void P2PConn::SendTraversalResult(const traversal_id_t& traversalId, int result)
{
	std::cout << "P2PConn::SendTraversalResult()" << endl;
	uint32_t seqno;
	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;
	protocalBuilder.Reset();
	protocalBuilder.AddRequestHead(PASSTHROUGH_CMD,
			CMD_TYPE_NORMAL,
			true,
			S_THE_DEVID,
			PS_SOURCE);
	protocalBuilder.AddDataPram(PK_subCmd, NATTraversalResu);
	protocalBuilder.AddDataPram(PK_traversalID, traversalId.id);
	protocalBuilder.AddDataPram(PK_Result, result);

	seqno = m_seq++;
	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	msgBuilder.MakePacket(
			protocalBuilder.Commit(),
			seqno,
		PS_MSG_TYPE);

	SendToPS(msgBuilder);

	// 更新重传请求
	CRefCountedBuffer buff;
	msgBuilder.GetResult(&buff);
	UpdateResendRequest(m_nsAddr, buff,	NATTraversalResu, true);
}

void P2PConn::HandleReq(P2PMessageReader& msgReader, CSocketAddress& fromAddr)
{
	static MSG_HANDLER msgHandler[] = {
		&P2PConn::HandleTraversalNotify,
		&P2PConn::HandleTraversalReq,
		&P2PConn::HandleTraversalResp,
		&P2PConn::HandleTraversalTry,
		&P2PConn::HandleTraversalTry2,
		&P2PConn::HandleTraversalTestAck,
		&P2PConn::HandleTraversalHB,
		&P2PConn::HandleTraversalStop,
		NULL,
	};

	static const char* cmdList[] = {
		NATTraversalNotify,
		NATTraversalReq,
		NATTraversalResp,
		NATTraversalTest1,
		NATTraversalTest2,
		NATTraversalTestAck,
		NATTraversalHB,
		NATTraversalStop,
		NULL,
	};

	string cmd = msgReader.SubCmd();

	DispatchRequest(msgHandler, cmdList, cmd.c_str(), msgReader, fromAddr);
}

bool P2PConn::DispatchRequest(MSG_HANDLER* pfnHandleList,
		const char* cmdList[],
		const char* cmd,
		P2PMessageReader& msgReader,
		CSocketAddress& fromAddr)
{
	size_t cmdlen = 0;
	if (cmd == NULL)
	{
		return false;
	}
	cmdlen = strlen(cmd);

	for (int i=0; cmdList[i] != NULL; i++)
	{
		if (cmdlen == strlen(cmdList[i])
				&& strcmp(cmd, cmdList[i]) == 0
				&& pfnHandleList[i] != NULL)
		{
			(this->*pfnHandleList[i])(msgReader, fromAddr);
			return true;
		}
	}

	return false;
}

void P2PConn::HandleAck(P2PMessageReader& msgReader, CSocketAddress& fromAddr)
{
	static MSG_HANDLER msgHandler[] = {
		&P2PConn::HandleTraversalReqAck,
		&P2PConn::HandleTraversalRespAck,
		&P2PConn::HandleTraversalResuAck,
		&P2PConn::HandleTraversalTestAckResp,
		&P2PConn::HandleTraversalHBAck,
		&P2PConn::HandleNodesTraversalRetryAck,
		NULL
	};

	static const char* cmdList[] = {
		NATTraversalReq,
		NATTraversalResp,
		NATTraversalResu,
		NATTraversalTestAck,
		NATTraversalHB,
		NodesTraversalRetry,
		NULL
	};

	string sRespCmd = msgReader.SubAck();

	DispatchRequest(msgHandler, cmdList, sRespCmd.c_str(), msgReader, fromAddr);
}

bool P2PConn::IsTraversaling()
{
	return m_state == WaitBindingTestResult
		|| m_state == WaitTraversalResponse
		|| m_state == WaitConnTry2
		|| m_state == WaitConnAck;
}

bool P2PConn::IsConnected()
{
	return m_state == Connected; 
}

void P2PConn::HandleTraversalNotify(P2PMessageReader& msgReader, CSocketAddress& fromAddr)
{
	HRESULT hr = S_OK;
	int ackCode = ACK_OK;
	string ackMsg = "OK";
	const NATTraversalNotifyParse *parse = (const NATTraversalNotifyParse*)msgReader.GetNatProtocalParse();
	m_nsAddr = fromAddr;

	if (parse == NULL)
	{
		SendResponse(msgReader.SeqNo(), 
				m_traversalId,
				NATTraversalNotify,
				ACK_FAIL, 
				"Protocal parse failed");
		return;
	}

	if (IsTraversaling())
	{
		// send response
		SendResponse(msgReader.SeqNo(), 
				parse->TraversalId(), 
				NATTraversalNotify,
				ackCode, 
				ackMsg.c_str());
		return;
	}

	if (IsConnected())
	{
		// send response
		SendResponse(msgReader.SeqNo(),  
				m_traversalId, 
				NATTraversalNotify,
				ACK_CONNECTED, 
				"Connected",
				true,
				parse->TraversalId().id
				);
		return;
	}

	// start traversl
	m_traversalId = parse->TraversalId();
	m_targetDevid = parse->TargetId();
	m_fMaster = true;
	hr = NumericIPToAddress(AF_INET, parse->StunSvrIpAddr(), &m_stunServerAddr);
	if (FAILED(hr))
	{
		printf("Invalid stun server ip: %s\n", parse->StunSvrIpAddr());
		ackCode = ACK_FAIL;
		ackMsg = "Invalid stun server ip: ";
		ackMsg += parse->StunSvrIpAddr();
	}
	else
	{	
		std::cout << "Stun server: " << parse->StunSvrIpAddr() << ":" << parse->StunSvrPort() << endl;
		m_stunServerAddr.SetPort(parse->StunSvrPort());

		hr = StartTraversal();
		if (FAILED(hr))
		{
			ackCode = ACK_FAIL;
			ackMsg = "Start traversal failed";
		}
	}

	hr = SendResponse(msgReader.SeqNo(), 
			parse->TraversalId(), 
			NATTraversalNotify,
			ackCode, 
			ackMsg.c_str());
	if (FAILED(hr))
	{
		printf("Send traversal notify response failed\n");
	}
}

void P2PConn::HandleTraversalReq(P2PMessageReader& msgReader, CSocketAddress& fromAddr)
{
	HRESULT hr = S_OK;
	int ackCode = ACK_OK;
	string ackMsg = "OK";
	const NATTraversalReqParse *parse = (const NATTraversalReqParse*)msgReader.GetNatProtocalParse();
	m_nsAddr = fromAddr;
	
	if (parse == NULL)
	{
		SendResponse(msgReader.SeqNo(), 
				m_traversalId,
				NATTraversalReq,
				ACK_FAIL, 
				"Parse NATTravesalReq failed");
		return;
	}

	if (IsTraversaling())
	{
		// send response
		SendResponse(msgReader.SeqNo(),  
				parse->TraversalId(), 
				NATTraversalReq,
				ackCode, 
				ackMsg.c_str());
		return;
	}

	if (IsConnected())
	{
		// send response
		SendResponse(msgReader.SeqNo(),  
				parse->TraversalId(), 
				NATTraversalReq,
				ackCode, 
				ackMsg.c_str());
		return;
	}

	if (FAILED(NumericIPToAddress(AF_INET, parse->MappedIpPP(), &m_peerMappedAddrPP))
	|| FAILED(NumericIPToAddress(AF_INET, parse->MappedIpAA(), &m_peerMappedAddrAA))
	|| FAILED(NumericIPToAddress(AF_INET, parse->LocalIp(), &m_peerLocalAddr))
	|| FAILED(NumericIPToAddress(AF_INET, parse->StunServerIp(), &m_stunServerAddr)))
	{
		ackCode = ACK_FAIL;
		ackMsg = "Invalid term mapped addr";
	}
	else
	{
		m_peerMappedAddrAA.SetPort(parse->MappedPortAA());
		m_peerMappedAddrPP.SetPort(parse->MappedPortPP());
		m_peerLocalAddr.SetPort(parse->LocalPort());
		m_stunServerAddr.SetPort(parse->StunServerPort());
		m_targetDevid = parse->DevId();
		m_fMaster = false;
		hr = StartTraversal();
		if (FAILED(hr))
		{
			ackCode = ACK_FAIL;
			ackMsg = "start traversal failed";
		}	
	}
	// Send response
	hr = SendResponse(msgReader.SeqNo(), 
			parse->TraversalId(), 
			NATTraversalReq,
			ackCode, 
			ackMsg.c_str());
	if (FAILED(hr))
	{
		printf("Send traversal req response failed\n");
	}
}

void P2PConn::HandleTraversalResp(P2PMessageReader& msgReader, CSocketAddress& fromAddr)
{
	int ackCode = ACK_OK;
	string ackMsg = "OK";
	const NATTraversalRespParse *parse = (const NATTraversalRespParse*)msgReader.GetNatProtocalParse();
	if (parse == NULL)
	{
		SendResponse(msgReader.SeqNo(),
				m_traversalId,
				NATTraversalResp,
				ACK_FAIL,
				"Parse Traversal Resp failed");
		return;
	}

	if (m_state != WaitTraversalResponse)
	{
		SendResponse(msgReader.SeqNo(),
				parse->TraversalId(), 
				NATTraversalResp,
				ACK_FAIL,
				"Wrong traversal state");
		return;
	}

	if (FAILED(NumericIPToAddress(AF_INET, parse->MappedIpPP(), &m_peerMappedAddrPP))
		|| FAILED(NumericIPToAddress(AF_INET, parse->MappedIpAA(), &m_peerMappedAddrAA))
		|| FAILED(NumericIPToAddress(AF_INET, parse->LocalIp(), &m_peerLocalAddr)))
	{
		ackCode = ACK_FAIL;
		ackMsg = "Invalid term mapped addr";
	}
	else
	{
		// send test2 request and update state to wait conn ack 
		UpdateState(WaitConnAck);
		if (FAILED(ForcastAndSendConnTry()))
		{
			ackCode = ACK_FAIL;
			ackMsg = "Forecast and send conn try failed";
			UpdateState(Error);
		}
	}

	// Send response
	SendResponse(msgReader.SeqNo(), 
			parse->TraversalId(), 
			NATTraversalResp,
			ackCode,
			ackMsg.c_str());
}
	
void P2PConn::HandleTraversalTry(P2PMessageReader& msgReader, CSocketAddress& fromAddr)
{
	string sAddr;
	fromAddr.ToString(&sAddr);
	std::cout << "Recv Traversal Try from " << sAddr << endl;
	m_peerAddr = fromAddr;
}

void P2PConn::HandleTraversalTry2(P2PMessageReader& msgReader, CSocketAddress& fromAddr)
{
	string sAddr;
	fromAddr.ToString(&sAddr);
	std::cout << "Recv Traversal Try from " << sAddr << endl;
	uint32_t seqno = 0;
	m_peerAddr = fromAddr;

	// send test req response
	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;

	protocalBuilder.Reset();
	protocalBuilder.AddRequestHead(PASSTHROUGH_CMD,
			CMD_TYPE_NORMAL,
			true,
			S_THE_DEVID,
			Devid2Str(m_targetDevid).c_str());	
	protocalBuilder.AddDataPram(PK_subCmd, NATTraversalTestAck);
	protocalBuilder.AddDataPram(PK_traversalID, m_traversalId.id);
	
	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	seqno = m_seq++;
	if(FAILED(msgBuilder.MakePacket(protocalBuilder.Commit(),
				seqno,
				PS_MSG_TYPE)))
	{
		return;
	}
	CRefCountedBuffer buff;
	msgBuilder.GetResult(&buff);
	SendCmd(buff->GetData(), buff->GetSize(), m_peerAddr);

	if (m_state != Connected)
	{
		UpdateState(WaitConnAckResp);
	}

	UpdateResendRequest(m_peerAddr, buff, NATTraversalTestAck, false);
}

void P2PConn::HandleTraversalTestAck(P2PMessageReader& msgReader, CSocketAddress& fromAddr)
{
	m_peerAddr = fromAddr;

	const NATProtocalParseBase *parse = (const NATProtocalParseBase*)msgReader.GetNatProtocalParse();
	if (parse == NULL)
	{
		SendResponse(msgReader.SeqNo(),
				m_traversalId,
				NATTraversalTestAck,
				ACK_FAIL,
				"Parse NATTraversalTestAck failed",
				false);
		return;
	}

	bool fConnected = (m_state == Connected);

	// update state	
	UpdateState(Connected);
	
	// send TraversalTestAck response to Peer
	SendResponse(msgReader.SeqNo(),
			parse->TraversalId(),
			NATTraversalTestAck,
			ACK_OK,
			"OK",
			false);

	// todo send traversal result to ps
	if (!fConnected)
	{
		SendTraversalResult(parse->TraversalId(), ACK_OK);
	}
}

void P2PConn::HandleTraversalHB(P2PMessageReader& msgReader, CSocketAddress& fromAddr)
{
	m_hbRecvTick = GetMillisecondCounter(); 
}


void P2PConn::HandleTraversalStop(P2PMessageReader& msgReader, CSocketAddress& fromAddr)
{
	std::cout << "Enter HandleTraversalStop " << m_traversalId.id << endl;
	const NATProtocalParseBase *parse = (const NATProtocalParseBase*)msgReader.GetNatProtocalParse();
	if (parse == NULL)
	{
		std::cout << "Parse NATTraversalStop failed" << endl;
		SendResponse(msgReader.SeqNo(),
				m_traversalId,
				NATTraversalStop,
				ACK_FAIL,
				"Parse NATTraversalStop failed");
		return;
	}
	SendResponse(msgReader.SeqNo(),
			m_traversalId,
			NATTraversalStop,
			ACK_OK,
			"OK");

	UpdateState(Disconnected);
}


// ack

void P2PConn::HandleTraversalReqAck(P2PMessageReader& msgReader, CSocketAddress& fromAddr)
{
	uint32_t seqno;

	seqno = msgReader.SeqNo();
	m_resendData.CheckResp(NATTraversalReq);
}

void P2PConn::HandleTraversalRespAck(P2PMessageReader& msgReader, CSocketAddress& fromAddr)
{
	uint32_t seqno;

	seqno = msgReader.SeqNo();
	m_resendData.CheckResp(NATTraversalResp);
}

void P2PConn::HandleTraversalResuAck(P2PMessageReader& msgReader, CSocketAddress& fromAddr)
{
	uint32_t seqno;

	seqno = msgReader.SeqNo();
	m_resendData.CheckResp(NATTraversalResu);

	m_fReportTestResult = true;
}

void P2PConn::HandleTraversalTestAckResp(P2PMessageReader& msgReader, CSocketAddress& fromAddr)
{
	uint32_t seqno;
	seqno = msgReader.SeqNo();
	m_resendData.CheckResp(NATTraversalTestAck);

	UpdateState(Connected);
}

void P2PConn::HandleTraversalHBAck(P2PMessageReader& msgReader, CSocketAddress& fromAddr)
{
	uint32_t seqno;

	seqno = msgReader.SeqNo();
	m_resendData.CheckResp(NATTraversalHB);
	
	m_hbRecvTick = GetMillisecondCounter(); 
}

void P2PConn::HandleNodesTraversalRetryAck(P2PMessageReader& msgReader, CSocketAddress& fromAddr)
{
	uint32_t seqno;

	seqno = msgReader.SeqNo();
	m_resendData.CheckResp(NodesTraversalRetry);
	
	m_fReportTestResult = true;
}

HRESULT P2PConn::StartTraversal()
{	
	HRESULT hr = S_OK;
	
	m_stunProcess.SetStunServerAddr(m_stunServerAddr);
	Chk(m_stunProcess.StartStunTest());

	UpdateState(WaitBindingTestResult);	

Cleanup:
	return hr;
}

HRESULT P2PConn::SendResponse(uint32_t seqNo,
		const traversal_id_t &traversalId, 
		const char* ackCmd,
		int code, 
		const char* msg,
		bool fToPS,
		const char* newTraversalId)
{
	HRESULT hr = S_OK;
	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;
	CRefCountedBuffer buff;
	
	protocalBuilder.Reset();
	protocalBuilder.AddRequestHead(PASSTHROUGH_CMD,
			CMD_TYPE_NORMAL,
			false,
			S_THE_DEVID,
			PS_SOURCE);
	protocalBuilder.AddDataPram(PK_subCmd, ResponseCmd);
	protocalBuilder.AddDataPram(PK_subACK, ackCmd);
	if (newTraversalId == NULL)
	{
		protocalBuilder.AddDataPram(PK_traversalID, traversalId.id);
	}
	else
	{
		protocalBuilder.AddDataPram(PK_traversalID, newTraversalId);
		protocalBuilder.AddDataPram(PK_oldTraversalID, traversalId.id);
	}
	protocalBuilder.AddDataPram(PK_retCode, code);
	protocalBuilder.AddDataPram(PK_retMsg, msg);

	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	Chk(msgBuilder.MakePacket(
			protocalBuilder.Commit(),
			seqNo,
		PS_MSG_TYPE));

//	std::cout << "P2PConn::SendResponse " << (char*)msgBuilder.GetStream().GetDataPointerUnsafe() + TB_HEAD_SIZE << endl;

	if (fToPS)
	{
		Chk(SendToPS(msgBuilder));
	}
	else
	{
		msgBuilder.GetStream().GetBuffer(&buff);
		ChkIf(P2P_OK != Send2Peer(buff->GetData(), buff->GetSize(), false), E_FAIL);
	}

Cleanup:
	return hr;
}

HRESULT P2PConn::SendToPS(P2PMessageBuilder& msgBuilder)
{
	HRESULT hr = S_OK;
	CRefCountedBuffer buff;
	uint32_t nsIp;
	char* p;

	//cout << "SendToPS 1" << endl;
	ChkIfA(m_pPSAgent == NULL, E_FAIL);
	//cout << "SendToPS 2" << endl;
	Chk(msgBuilder.GetResult(&buff));
	//cout << "SendToPS 3" << endl;
	m_nsAddr.GetIP(&nsIp, sizeof(nsIp));
	Chk(m_pPSAgent->SendData(buff->GetData(), buff->GetSize(), 
			nsIp, m_nsAddr.GetPort()));
	//cout << "SendToPS 4" << endl;

	p = (char*)buff->GetData();
	p[buff->GetSize()] = 0;
	std::cout << "SendToPS: " << (char*)buff->GetData() + TB_HEAD_SIZE << endl;
Cleanup:
	return hr;
}

void P2PConn::UpdateState(P2PConnState state)
{
	m_stateTick = GetMillisecondCounter(); 
	if (state == m_state)
	{
		return;
	}
	m_state = state;

	if (m_pP2PAgent == NULL)
	{
		return;
	}

	switch (state)
	{
	case WaitBindingTestResult:
		m_pP2PAgent->OnConnEvent(p2p_connecting, m_targetDevid);
		break;
	case Connected:
		m_pP2PAgent->OnConnEvent(p2p_connected, m_targetDevid);
		break;
	case Timeout:
		m_pP2PAgent->OnConnEvent(p2p_failed, m_targetDevid);
		break;
	case Disconnected:
		m_pP2PAgent->OnConnEvent(p2p_disconnected, m_targetDevid);
		break;
	default:
		break;
	}
}

void P2PConn::CheckResendRequest()
{
	CSocketAddress destAddr;
	CRefCountedBuffer buff;
	bool fToPs = false;

	if (m_resendData.NeedResendData(destAddr,
				buff,
				fToPs))
	{
		if (fToPs && m_pPSAgent != NULL)
		{
			if (m_pPSAgent != NULL)
			{
				uint32_t nsIp;
				destAddr.GetIP(&nsIp, sizeof(nsIp));
				m_pPSAgent->SendData(buff->GetData(), buff->GetSize(), 
					nsIp, destAddr.GetPort());
			}
		}
		else
		{
			int sendret = -1;
			int sendsock = -1;

			sendsock = m_sock.GetSocketHandle();
			if (sendsock == -1)
			{	
				return; 
			}

			sendret = ::sendto(sendsock, 
					buff->GetData(), 
					buff->GetSize(),
					0, 
					destAddr.GetSockAddr(), 
					destAddr.GetSockAddrLength());
		}
	}
}

void P2PConn::UpdateResendRequest(CSocketAddress& destAddr,
		CRefCountedBuffer& data,
		const char* cmd,
		bool fToPs)
{
	m_resendData.Reset(destAddr, data, cmd, fToPs);
}

HRESULT P2PConn::ForcastAndSendConnTry()
{
	HRESULT hr = S_OK;
	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;
	CRefCountedBuffer buff;
	uint16_t portPP = m_peerMappedAddrPP.GetPort();
	uint16_t portAA = m_peerMappedAddrAA.GetPort();
	CSocketAddress addr = m_peerMappedAddrPP;
	uint16_t portStart = 0;
	uint16_t interval = 1;
	int count = P2P_FORCAST_PACKET_COUNT;
	int i = 0;
	uint32_t seqno = 0;

	protocalBuilder.Reset();
	protocalBuilder.AddRequestHead(PASSTHROUGH_CMD,
			CMD_TYPE_NORMAL,
			true,
			S_THE_DEVID,
			Devid2Str(m_targetDevid).c_str());	
	if (m_fMaster)
	{
		protocalBuilder.AddDataPram(PK_subCmd, NATTraversalTest2);
	}
	else
	{
		protocalBuilder.AddDataPram(PK_subCmd, NATTraversalTest1);
	}
	protocalBuilder.AddDataPram(PK_traversalID, m_traversalId.id);
	
	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	seqno = m_seq++;
	Chk(msgBuilder.MakePacket(protocalBuilder.Commit(),
				seqno,
				PS_MSG_TYPE));
	msgBuilder.GetStream().GetBuffer(&buff);
	
	Chk(msgBuilder.GetResult(&buff));
	if (m_peerAddr.GetPort() != 0)
	{
		SendCmd(buff->GetData(), buff->GetSize(), m_peerAddr);
		return hr;
	}

	// send to peer local addr
	SendCmd(buff->GetData(), buff->GetSize(), m_peerLocalAddr);

	// 对端NAT behavior: DirectMapping
	if (m_peerLocalAddr.IsSameIP_and_Port(m_peerMappedAddrPP))
	{
		return hr;
	}

	// 对端NAT behavior: EndpointIndependentMapping
	if (m_peerMappedAddrPP.IsSameIP_and_Port(m_peerMappedAddrAA))
	{
		SendCmd(buff->GetData(), buff->GetSize(), m_peerMappedAddrPP);
		return hr;
	}

	// 对端NAT behavior: AddressDependentMapping or AddressAndPortDependentMapping
	if (m_peerMappedAddrPP.GetPort() == m_peerMappedAddrAA.GetPort())
	{
		SendCmd(buff->GetData(), buff->GetSize(), m_peerMappedAddrPP);
		SendCmd(buff->GetData(), buff->GetSize(), m_peerMappedAddrAA);
	}
	else
	{
		count = P2P_FORCAST_PACKET_COUNT;
		if (portPP < portAA)
		{
			interval = 1;
			portStart = portAA + 1;
			if (portPP + 1 == portAA)
			{
				count *= 2;
			}
		}
		else
		{
			interval = -1;
			portStart = portAA - 1;
			if (portPP - 1 != portAA)
			{
				count *= 2;
			}
		}
		for (i=0; i<count; i++)
		{
			addr = m_peerMappedAddrPP;
			addr.SetPort(portStart);
			SendCmd(buff->GetData(), buff->GetSize(), addr);
			if (!m_peerMappedAddrPP.IsSameIP(m_peerMappedAddrAA))
			{
				addr = m_peerMappedAddrAA;
				addr.SetPort(portStart);
				SendCmd(buff->GetData(), buff->GetSize(), addr);
			}
			portStart += interval;
		}
	}

Cleanup:
	return hr;
}

bool P2PConn::IsDead()
{
	return ((m_state == Timeout
			|| m_state == Error
			|| m_state == Stopped
			|| m_state == InitState)
			&& GetMillisecondCounter() - m_stateTick > 30000);
//			&& m_fReportTestResult);
}

////////////////////////////////////////////////////
//


ResendData::ResendData() : m_retry(0),
	m_lastSendTick(0),
	m_fToPs(true),
	m_seqno(0)
{
	m_buff = CRefCountedBuffer(new CBuffer(MAX_PACKET_LENGTH));
}

inline void ResendData::CheckResp(uint32_t seqno)
{
	if (seqno == m_seqno)
	{
		std::cout << "CheckResp seqno: " << seqno << endl;
		m_seqno = 0;	
	}
	else
	{
		std::cout << "CheckResp seqno error, recved:" << seqno << " expected: " << m_seqno
			<< endl;
	}
}

inline void ResendData::Reset(CSocketAddress& destAddr,
		CRefCountedBuffer& data,
		uint32_t seqno,
		bool fToPs)
{
	m_stream.Reset();
	m_stream.Attach(m_buff, true);
	m_stream.Write(data->GetData(), data->GetSize());
	m_destAddr = destAddr;
	m_fToPs = fToPs;
	m_retry = 0;
	m_seqno = seqno;
	m_cmd = "";
	m_lastSendTick = GetMillisecondCounter();
	std::cout << "Resend seqno: " << seqno << " cmd:" << m_cmd << endl;
}

inline void ResendData::CheckResp(const char* cmd)
{
	assert(cmd != NULL);
	if (m_cmd.compare(cmd) == 0)
	{
		std::cout << "CheckResp cmd: " << m_cmd << endl;
		m_cmd = "";	
	}
	else
	{
		std::cout << "CheckResp cmd error, recved:" << cmd << " expected: " << m_cmd
			<< endl;
	}
}

inline void ResendData::Reset(CSocketAddress& destAddr,
		CRefCountedBuffer& data,
		const char* cmd,
		bool fToPs)
{
	m_stream.Reset();
	m_stream.Attach(m_buff, true);
	m_stream.Write(data->GetData(), data->GetSize());
	m_destAddr = destAddr;
	m_fToPs = fToPs;
	m_retry = 0;
	m_seqno = 0;
	m_cmd = cmd;
	m_lastSendTick = GetMillisecondCounter();
	std::cout << "Resend cmd: " << m_cmd << " seqno: " << m_seqno << endl;
}

inline bool ResendData::NeedResendData(CSocketAddress& destAddr,
		CRefCountedBuffer& data,
		bool& fToPs)
{
	if (m_seqno == 0)
	{
		return false;
	}
	if (m_retry >= REQ_MAX_RESEND_COUNT
			|| GetMillisecondCounter() - m_lastSendTick < REQ_RESEND_INTERVEL)
	{
		return false;
	}

	m_retry++;
	m_lastSendTick = GetMillisecondCounter();
	destAddr = m_destAddr;
	data = m_buff;
	fToPs = m_fToPs;
	return true;
}



