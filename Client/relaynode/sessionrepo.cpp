/*************************************************************************
    > File Name: sessionrepo.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2016年01月05日 星期二 16时20分54秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "sessionrepo.h"
#include "oshelper.h"
#include <iostream>

using namespace std;

SessionRepo* SessionRepo::m_inst = NULL;

SessionRepo::SessionRepo() : m_lastCheckSessionStateTick(0),
	m_sessionHandle(0)
{
	m_sessionMap.InitTable(1000, 37);
}

SessionRepo::~SessionRepo()
{
	Clear();
}

SessionRepo* SessionRepo::Instance()
{
	if (m_inst == NULL)
	{
		m_inst = new SessionRepo();
	}
	return m_inst;
}

void SessionRepo::Destory()
{
	if (m_inst != NULL)
	{
		delete m_inst;
		m_inst = NULL;
	}
}

Session* SessionRepo::CreateSession(string& sessionId, string& url)
{
	uint32_t sessionHandle = ++m_sessionHandle;
	Session* sess = new Session(sessionId.c_str(), url.c_str(), sessionHandle);
	if (sess->StartStream())
	{
		AddSession(sess);
		return sess;
	}
	else
	{
		delete sess;
		return NULL;
	}
}

void SessionRepo::DeleteSession(string& sessionId)
{
	Session* psess = GetSession(sessionId);
	if (psess != NULL)
	{
		RmSession(psess);
		psess->StopStream();
		delete psess;
	}
}

void SessionRepo::AddSession(Session* sess)
{
	assert(sess != NULL);
	session_id_t sid;
	sid.id = sess->Id();

	MyLock lock(&m_lock);
	lock.SetBusy();

	m_sessionMap.Insert(sid, sess);
	std::cout << "SessionRepo::AddSession, total:" << m_sessionMap.Size() << endl;
}

void SessionRepo::RmSession(Session* sess)
{
	assert(sess != NULL);
	session_id_t sid;
	sid.id = sess->Id();

	MyLock lock(&m_lock);
	lock.SetBusy();

	m_sessionMap.Remove(sid);
	std::cout << "SessionRepo::RmSession, total:" << m_sessionMap.Size() << endl;
}

Session* SessionRepo::GetSession(string& sessionId)
{
	MyLock lock(&m_lock);
	lock.SetBusy();

	session_id_t sid;
	sid.id = sessionId;

	Session** result = m_sessionMap.Lookup(sid);
	if (result == NULL)
	{
		return NULL;
	}
	return *result;
}

void SessionRepo::OnTimer()
{
	CheckSessionState();
}

void SessionRepo::CheckSessionState()
{
	const uint32_t SessionCheckInterval = 5000;  // 5秒

	MyLock lock(&m_lock);
	if (!lock.TestAndSetBusy())
	{
		return;
	}
	size_t count = m_sessionMap.Size();
	for (size_t i = 0; i < count; i++)
	{
		session_map_t::Item *item = m_sessionMap.LookupByIndex(i);
		if (item != NULL)
		{
			Session* psess = item->value;
			psess->OnTimer();
		}
	}

	// 检查过期的会话
	if (GetMillisecondCounter() - m_lastCheckSessionStateTick < SessionCheckInterval)
	{
		return;
	}

	for (size_t i = 0; i < count; i++)
	{
		session_map_t::Item *item = m_sessionMap.LookupByIndex(i);
		if (item != NULL)
		{
			Session* psess = item->value;
			if (psess->State() == Session::Timeout)
			{
				m_sessionMap.Remove(item->key);
				psess->StopStream();
				delete psess;
				break;				// 一次只删除一个会话
			}
		}
	}

	m_lastCheckSessionStateTick = GetMillisecondCounter();
}

void SessionRepo::Clear()
{
	MyLock lock(&m_lock);
	lock.SetBusy();

	size_t count = m_sessionMap.Size();
	for (size_t i = 0; i < count; i++)
	{
		session_map_t::Item *item = m_sessionMap.LookupByIndex(i);
		if (item != NULL)
		{
			Session* psess = item->value;
			psess->StopStream();
			delete psess;
		}
	}
	m_sessionMap.Reset();
}


