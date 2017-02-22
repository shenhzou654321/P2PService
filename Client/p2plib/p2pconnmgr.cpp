/*************************************************************************
    > File Name: p2pconnmgr.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月25日 星期三 14时34分57秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "p2plibimpl.h"
#include "psagent.h"
#include "p2pconnmgr.h"
#include <vector>
#include <iostream>

using namespace std;

P2PConnMgr::P2PConnMgr() : 
	m_pthread((pthread_t)-1),
	m_fNeedToExit(false),
	m_fThreadIsValid(false),
	m_pP2PAgentImpl(NULL),
	m_pPSAgent(NULL),
	m_spMsgBuf(new CBuffer(MAX_PACKET_LENGTH + 2))
{
	m_p2pConnMap.InitTable(1000, 37);
	m_devIdP2PConnMap.InitTable(1000, 37);
}

P2PConnMgr::~P2PConnMgr()
{
	Stop();
	Clear();
}

HRESULT P2PConnMgr::Init(P2PAgentImpl* pP2PAgentImpl, PSAgent* pPSAgent)
{
	m_pP2PAgentImpl = pP2PAgentImpl;
	m_pPSAgent = pPSAgent;
	return S_OK;
}

void P2PConnMgr::Clear()
{
	MyLock lock(&m_lock);
	lock.SetBusy();
	m_p2pConnMap.Reset();
	m_devIdP2PConnMap.Reset();
}

HRESULT P2PConnMgr::Start()
{
	HRESULT hr = S_OK;
	int err = 0;

	ChkIf(m_fThreadIsValid, E_UNEXPECTED);

	err = ::pthread_create(&m_pthread, NULL, P2PConnMgr::ThreadFunction, this);

	ChkIf(err != 0, ERRNO_TO_HRESULT(err));
	m_fThreadIsValid = true;

Cleanup:
	return hr;
}

void P2PConnMgr::Stop()
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

void* P2PConnMgr::ThreadFunction(void* pThis)
{
	((P2PConnMgr*)pThis)->Run();
	return NULL;
}

void P2PConnMgr::Run()
{
	const uint32_t CONN_CLEAN_INTERVAL = 30000; // 30s
	uint32_t cleanTick = GetMillisecondCounter();
/*	traversal_id_t traversalId;
	devid_t targetId = 1;
	RefCountedP2PConn p2pConn;
	memset(traversalId.id, 0, 32);
*/

	while (!m_fNeedToExit)
	{
		// sleep 100 ms
		usleep(100000);
/*
		// Test codes for P2PConn object management
		// create conn obj
		sprintf(traversalId.id, "%d", GetMillisecondCounter());
		targetId++;
		p2pConn = RefCountedP2PConn(new P2PConn());
		if (!FAILED(p2pConn->Init(traversalId, m_pPSAgent, m_pP2PAgentImpl)))
		{
			AddP2PConn(traversalId, targetId,  p2pConn);
		}
*/			

		// p2p session ontime
		size_t size = m_p2pConnMap.Size();

		for (size_t i=0; i<size; i++)
		{
			RefCountedP2PConn *pP2PConn = m_p2pConnMap.LookupValueByIndex(i);
			(*pP2PConn)->OnTick();
		}

		if (GetMillisecondCounter() - cleanTick > CONN_CLEAN_INTERVAL)
		{
			CleanStoppedConn();
			cleanTick = GetMillisecondCounter();
		}
	}
}

void P2PConnMgr::CleanStoppedConn()
{
	vector<RefCountedP2PConn> deadConnList;

	do
	{
		MyLock lock(&m_lock);
		if (!lock.TestAndSetBusy())
		{
			return;
		}
		size_t count = m_p2pConnMap.Size();
		for (size_t i=0; i<count; i++)
		{
			P2PConnMap::Item *item = m_p2pConnMap.LookupByIndex(i);
			if (item != NULL && item->value->IsDead())
			{
				deadConnList.push_back(item->value);
				m_p2pConnMap.Remove(item->key);

				m_devIdP2PConnMap.Remove(item->value->DevId());

				break;
			}
		}	
	}while(0);

	deadConnList.clear();
}

bool P2PConnMgr::GetP2PConn(const traversal_id_t& traversalId, RefCountedP2PConn& p2pconn)
{
	MyLock lock(&m_lock);
	lock.SetBusy();

	RefCountedP2PConn *pP2PConn = NULL;
	pP2PConn = m_p2pConnMap.Lookup(traversalId);
	if (pP2PConn == NULL)
	{
		return false;
	}
	p2pconn = *pP2PConn;
	return true;
}

bool P2PConnMgr::GetP2PConn(const devid_t& devid, RefCountedP2PConn& p2pconn)
{
	MyLock lock(&m_lock);
	lock.SetBusy();
	RefCountedP2PConn *pP2PConn = NULL;
	pP2PConn = m_devIdP2PConnMap.Lookup(devid);
	if (pP2PConn == NULL)
	{
		return false;
	}
	p2pconn = *pP2PConn;
	return true;
}

void P2PConnMgr::AddP2PConn(const traversal_id_t& traversalId, 
		const devid_t& devid, 
		RefCountedP2PConn& p2pconn)
{
	MyLock lock(&m_lock);
	lock.SetBusy();
	m_p2pConnMap.Insert(traversalId, p2pconn);
	m_devIdP2PConnMap.Insert(devid, p2pconn);

	std::cout << "Conn obj count: " << m_p2pConnMap.Size() << ":" << m_devIdP2PConnMap.Size() << endl;
}

void P2PConnMgr::HandleTraversalMsg(P2PMessageReader& msgReader, CSocketAddress &fromAddr)
{
	const NATProtocalParseBase* pNatProtocalParse = NULL;
	RefCountedP2PConn p2pConn;
	devid_t targetId = (devid_t)-1;
	m_nsAddr = fromAddr;

	msgReader.Print();
	pNatProtocalParse = (const NATProtocalParseBase*)msgReader.GetNatProtocalParse();

	ASSERT(pNatProtocalParse != NULL);

	if (pNatProtocalParse == NULL)
	{
		std::cout << "Nat protocal parse fail" << endl;
		return;
	}

	const traversal_id_t& traversalId = pNatProtocalParse->TraversalId();

	if (!GetP2PConn(traversalId, p2pConn))
	{
		// 穿透通知
		if (strcmp(msgReader.SubCmd(), NATTraversalNotify) == 0)
		{
			NATTraversalNotifyParse* pParse = (NATTraversalNotifyParse*)pNatProtocalParse;
			targetId = pParse->TargetId();
		}
		else if (strcmp(msgReader.SubCmd(), NATTraversalReq) == 0)
		{
			NATTraversalReqParse* pParse = (NATTraversalReqParse*)pNatProtocalParse;
			targetId = pParse->DevId();
		}
		else if (strcmp(msgReader.SubCmd(), NATTraversalStop) == 0)
		{
			SendResponse(msgReader.SeqNo(),
					traversalId,
					msgReader.SubCmd(),
					ACK_OK,
					"Connection was closed.");
			return;
		}
		else
		{
			std::cout << "Invalid sub cmd for new traversalId: " << msgReader.SubCmd() << endl;
		}
		
		if (targetId != (devid_t)-1)
		{
			// 检查是否已经与指定设备建立连接
			if (GetP2PConn(targetId, p2pConn))
			{
				if (p2pConn->IsConnected())
				{
					p2pConn->HandleTraversalMsg(msgReader, fromAddr);
					return;
				}
			}

			// create conn obj
			p2pConn = RefCountedP2PConn(new P2PConn());
			if (FAILED(p2pConn->Init(traversalId, m_pPSAgent, m_pP2PAgentImpl)))
			{
				std::cout << "P2PConn init failed" << endl;
				return;
			}
			else
			{
				AddP2PConn(traversalId, targetId,  p2pConn);
			}
		}
		else
		{
			// 请求报文 
			if (strcmp(msgReader.SubCmd(), ResponseCmd) != 0)
			{
				SendResponse(msgReader.SeqNo(),
					traversalId,
					msgReader.SubCmd(),
					ACK_FAIL,
					"Invalid traversal id");
			}
			return;
		}
	}

	if (p2pConn == NULL)
	{
		return;
	}
	p2pConn->HandleTraversalMsg(msgReader, fromAddr);
}

uint32_t P2PConnMgr::Send2Peer(const void *buff, int size, const devid_t& destDevId)
{
	uint32_t ret = P2P_OK;
	RefCountedP2PConn p2pConn;

	if (!GetP2PConn(destDevId, p2pConn))
	{
		return P2P_NOT_CONNECTED;
	}
	else
	{
		ret = p2pConn->Send2Peer(buff, size, true);
	}
	return ret;
}

uint32_t P2PConnMgr::GetConnectDevs(devid_list_t& devidList)
{
	MyLock lock(&m_lock);
	lock.SetBusy();
	size_t size = m_devIdP2PConnMap.Size();

	for (size_t i = 0; i < size; i++)
	{
		DevIdP2PConnMap::Item* pItem = m_devIdP2PConnMap.LookupByIndex(i);
		if (pItem != NULL && pItem->value->IsConnected())
		{
			devidList.push_back(pItem->key);
		}
	}
	return P2P_OK;
}

HRESULT P2PConnMgr::SendResponse(uint32_t seqNo,
		const traversal_id_t &traversalId, 
		const char* ackCmd,
		int code, 
		const char* msg)
{
	HRESULT hr = S_OK;
	P2PMessageBuilder msgBuilder;
	ProtocalBuilder protocalBuilder;

	protocalBuilder.Reset();
	protocalBuilder.AddRequestHead(PASSTHROUGH_CMD,
			CMD_TYPE_NORMAL,
			true,
			S_THE_DEVID,
			PS_SOURCE);
	protocalBuilder.AddDataPram(PK_subCmd, ResponseCmd);
	protocalBuilder.AddDataPram(PK_subACK, ackCmd);
	protocalBuilder.AddDataPram(PK_traversalID, traversalId.id);
	protocalBuilder.AddDataPram(PK_retCode, code);
	protocalBuilder.AddDataPram(PK_retMsg, msg);

	msgBuilder.GetStream().Attach(m_spMsgBuf, true);
	Chk(msgBuilder.MakePacket(
			protocalBuilder.Commit(),
			seqNo,
		PS_MSG_TYPE));

	Chk(SendToPS(msgBuilder));

Cleanup:
	return hr;
}

HRESULT P2PConnMgr::SendToPS(P2PMessageBuilder& msgBuilder)
{
	HRESULT hr = S_OK;
	CRefCountedBuffer buff;
	uint32_t nsIp;

	ChkIfA(m_pPSAgent == NULL, E_FAIL);

	Chk(msgBuilder.GetResult(&buff));

	m_nsAddr.GetIP(&nsIp, sizeof(nsIp));
	Chk(m_pPSAgent->SendData(buff->GetData(), buff->GetSize(), 
			nsIp, m_nsAddr.GetPort()));

	std::cout << "Send to PS: " << (char*)buff->GetData() + TB_HEAD_SIZE << endl;

Cleanup:
	return hr;
}


