/*************************************************************************
    > File Name: sessionrepo.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2016年01月05日 星期二 15时19分18秒
 ************************************************************************/

#ifndef _RELAYNODE_SESSIONREPO_H_
#define _RELAYNODE_SESSIONREPO_H_

#include "session.h"
#include "spinlock.h"
#include "fasthash.h"
#include <string>

using namespace std;

struct session_id_t
{
	string id;
	bool operator==(const session_id_t& other)
	{
		return id == other.id;
	}
};

inline size_t FastHash_Hash(const session_id_t& sessionId)
{
	if (sessionId.id.length() != 32)
	{
		return 0;
	}
	size_t result;
	if (sizeof(size_t) >= 8)
	{
		const uint64_t* pTemp = (uint64_t*)sessionId.id.c_str();
		result = (size_t)(pTemp[0] ^ pTemp[1] ^ pTemp[2] ^ pTemp[3]);
	}
	else
	{
		const uint32_t* pTemp = (uint32_t*)sessionId.id.c_str();
		result = (size_t)(pTemp[0] ^ pTemp[1] ^ pTemp[2] ^ pTemp[3]
				^ pTemp[4] ^ pTemp[5] ^ pTemp[6] ^ pTemp[7]);
	}
	return result;
};

class SessionRepo
{
public:
	~SessionRepo();

	static SessionRepo* Instance();
	static void Destory();

	Session* GetSession(string& sessionId);
	void DeleteSession(string& sessionId);
	Session* CreateSession(string& sessionId, string& url);
	void OnTimer();

private:
	SessionRepo();
	void CheckSessionState();
	void AddSession(Session* sess);
	void RmSession(Session* sess);
	void Clear();

private:
	static SessionRepo* m_inst;
	typedef FastHashDynamic<session_id_t, Session*> session_map_t;
	session_map_t m_sessionMap;
	SpinLock m_lock;
	uint32_t m_lastCheckSessionStateTick;
	uint32_t m_sessionHandle;
};

#endif // _RELAYNODE_SESSIONREPO_H_


