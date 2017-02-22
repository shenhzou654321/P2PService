/*************************************************************************
    > File Name: p2pconnmgr.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月25日 星期三 10时21分17秒
 ************************************************************************/

#ifndef _P2PCONNMGR_H_
#define _P2PCONNMGR_H_

#include "fasthash.h"
#include "p2pconn.h"
#include "spinlock.h"
#include <iostream>

using namespace std;

class P2PAgentImpl;
class PSAgent;

inline size_t FastHash_Hash(const traversal_id_t& traversalId)
{
//	std::cout << "Enter FastHash_Hash " << traversalId.id << endl;
	size_t result;
	if (sizeof(size_t) >= 8)
	{
		uint64_t* pTemp = (uint64_t*)(traversalId.id);
		result = (size_t)(pTemp[0] ^ pTemp[1] ^ pTemp[2] ^ pTemp[3]);
	}
	else
	{
		uint32_t* pTemp = (uint32_t*)(traversalId.id);
		result = (size_t)(pTemp[0] ^ pTemp[1] ^ pTemp[2] ^ pTemp[3] 
				^ pTemp[4] ^ pTemp[5] ^ pTemp[6] ^ pTemp[7]);
	}
//	std::cout << "Hash value: " << traversalId.id << ":" << result << endl;
	return result;
};

inline size_t FastHash_Hash(const devid_t& devid)
{
	return (size_t)devid;
}

class P2PConnMgr
{
public:
	P2PConnMgr();
	~P2PConnMgr();

	HRESULT Init(P2PAgentImpl* pP2PAgentImpl, PSAgent* pPSAgent);

	HRESULT Start();
	void Stop();

	void HandleTraversalMsg(P2PMessageReader& msgReader, CSocketAddress& fromAddr);

	uint32_t Send2Peer(const void *buff, int size, const devid_t& destDevId);

	uint32_t GetConnectDevs(devid_list_t& devidList);

private:
	void Run();
	static void* ThreadFunction(void* pThis);

	void Clear();

	void CleanStoppedConn();

	bool GetP2PConn(const traversal_id_t& traversalId, RefCountedP2PConn& p2pconn);
	bool GetP2PConn(const devid_t& devid, RefCountedP2PConn& p2pconn);
	void AddP2PConn(const traversal_id_t& traversalId, 
			const devid_t& devid, RefCountedP2PConn& p2pconn);
	HRESULT SendResponse(uint32_t seqNo,
		const traversal_id_t &traversalId, 
		const char* ackCmd,
		int code, 
		const char* msg);

	HRESULT SendToPS(P2PMessageBuilder& msgBuilder);


private:
	pthread_t m_pthread;
	bool m_fNeedToExit;
	bool m_fThreadIsValid;

	typedef FastHashDynamic<traversal_id_t, RefCountedP2PConn> P2PConnMap;
	typedef FastHashDynamic<devid_t, RefCountedP2PConn> DevIdP2PConnMap;
	P2PConnMap m_p2pConnMap;
	DevIdP2PConnMap m_devIdP2PConnMap;
	P2PAgentImpl* m_pP2PAgentImpl;
	PSAgent* m_pPSAgent;
	SpinLock m_lock;
	CSocketAddress m_nsAddr;
	CRefCountedBuffer m_spMsgBuf;
};

#endif // _P2PCONNMGR_H_


