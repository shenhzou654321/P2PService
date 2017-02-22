/*************************************************************************
    > File Name: psagent.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月20日 星期五 15时37分15秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "p2plibimpl.h"
#include "psagent.h"
#include "p2pconnmgr.h"


PSAgent::PSAgent() : m_pP2PAgentImpl(NULL)
{

}

PSAgent::~PSAgent()
{
	Close();
}

HRESULT PSAgent::Init(CSocketAddress &bindAddr, P2PAgentImpl* pP2PAgentImpl)
{
	HRESULT hr = S_OK;
	
	Chk(m_sock.UDPInit(bindAddr, this));

	m_pP2PAgentImpl = pP2PAgentImpl;

Cleanup:
	return hr;
}

void PSAgent::Close()
{
	m_sock.Close();
}

P2PSocket* PSAgent::GetP2PSocket()
{
	return &m_sock;
}

HRESULT PSAgent::SendData(const void *buff,
		int size,
		uint32_t destIpHostByteOrder,
		uint16_t destPort)
{
	HRESULT hr = S_OK;
	int sendret = -1;
	int sendsock = -1;
	CSocketAddress destAddr = CSocketAddress(destIpHostByteOrder, destPort);
	string sDestAddr;
	destAddr.ToString(&sDestAddr);

	sendsock = m_sock.GetSocketHandle();
	if (sendsock == -1)
	{
		printf("PSAgent::SendData() sendsock == -1\n");
	}
	ChkIf(sendsock == -1, E_FAIL);

	sendret = ::sendto(sendsock, buff, size, 0, destAddr.GetSockAddr(), destAddr.GetSockAddrLength());
    if (sendret <= 0)
	{
		printf("send %d bytes to %s, ret:%d, %d\n", size, sDestAddr.c_str(), sendret, errno);	
	}
	ChkIf(sendret <= 0, E_FAIL);

Cleanup:
	return hr;
}

void PSAgent::HandleMsg(CRefCountedBuffer &spBufferIn,
		CSocketAddress &fromAddr,
		CSocketAddress &localAddr,
		int sock)
{
	// printf("PSAgent::HandleMsg\n");
	uint32_t ip = 0;

	if (m_pP2PAgentImpl == NULL)
	{
		return;
	}
	m_msgReader.Reset();
	m_msgReader.AddBytes(spBufferIn->GetData(), spBufferIn->GetSize());
	if (m_msgReader.GetState() != P2PMessageReader::BodyValidated)
	{
	//	printf("Call P2PAgentImpl::HandleNSData\n");
		// wrong packet send to caller?
		fromAddr.GetIP(&ip, sizeof(ip));
		m_pP2PAgentImpl->HandleNSData(spBufferIn->GetData(), 
				spBufferIn->GetSize(), 
				ip,
				fromAddr.GetPort());
		return;
	}

	if (/*m_msgReader.CmdType() == PS_MSG_TYPE
		&& */strcmp(m_msgReader.Cmd(), PASSTHROUGH_CMD) == 0
		&& strcmp(m_msgReader.Source(), PS_SOURCE) == 0)
	{
		// handle p2p cmd
		HandleP2PCmd(fromAddr);
		printf("Handle p2p cmd\n");
	}
	else
	{
	//	 printf("Call P2PAgentImpl::HandleNSData\n");
		fromAddr.GetIP(&ip, sizeof(ip));
		m_pP2PAgentImpl->HandleNSData(spBufferIn->GetData(), 
				spBufferIn->GetSize(), 
				ip,
				fromAddr.GetPort());
	}
}

void PSAgent::HandleP2PCmd(CSocketAddress &fromAddr)
{
	P2PConnMgr* pp2pConnMgr = m_pP2PAgentImpl->GetP2PConnMgr();
	if (pp2pConnMgr != NULL)
	{
		pp2pConnMgr->HandleTraversalMsg(m_msgReader, fromAddr);
	}
}

