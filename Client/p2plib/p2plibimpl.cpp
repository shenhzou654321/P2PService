/*************************************************************************
    > File Name: p2plibimpl.cpp
    > Author: PYW 
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月20日 星期五 14时36分33秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "p2plibimpl.h"
#include "p2psocketthread.h"
#include "psagent.h"
#include "p2pconnmgr.h"
#include "deviceproxy.h"

P2PAgentImpl::P2PAgentImpl(p2p_config_t &config) :
	m_pP2PHandle(NULL),
	m_pP2PSocketThread(NULL),
	m_pPsAgent(NULL),
	m_pP2PConnMgr(NULL),
	m_pDeviceProxy(NULL)
{
	m_p2pconfig = config;
	Init();
}

P2PAgentImpl::~P2PAgentImpl()
{
	Stop();
	Clear();
}

uint32_t P2PAgentImpl::Init()
{
	m_pP2PSocketThread = new P2PSocketThread();
	m_pPsAgent = new PSAgent();
	m_pP2PConnMgr = new P2PConnMgr();

	if (!g_isRelayNode)
	{
		m_pDeviceProxy = new DeviceProxy();
	}

	return P2P_OK;
}

uint32_t P2PAgentImpl::Clear()
{
	if (m_pP2PSocketThread != NULL)
	{
		delete m_pP2PSocketThread;
		m_pP2PSocketThread = NULL;
	}

	if (m_pPsAgent != NULL)
	{
		delete m_pPsAgent;
		m_pPsAgent = NULL;
	}

	if (m_pP2PConnMgr != NULL)
	{
		delete m_pP2PConnMgr;
		m_pP2PConnMgr = NULL;
	}

	if (m_pDeviceProxy != NULL)
	{
		delete m_pDeviceProxy;
		m_pDeviceProxy = NULL;
	}

	return P2P_OK;
}

void P2PAgentImpl::RegisterHandle(IP2PHandle *handle)
{
	MyLock lock(&m_lock);
	lock.SetBusy();
	m_pP2PHandle = handle;
}

void P2PAgentImpl::UnRegisterHandle()
{
	MyLock lock(&m_lock);
	lock.SetBusy();
	m_pP2PHandle = NULL;
}

uint32_t P2PAgentImpl::Start(uint32_t bindIpHostByteOrder, uint16_t bindPort)
{
	HRESULT hr = S_OK;
	uint32_t ret = P2P_OK; 
	m_localAddress = CSocketAddress(bindIpHostByteOrder, bindPort);

	if (m_pDeviceProxy != NULL)
	{
		m_pDeviceProxy->Init(this);
		if (!SUCCEEDED(m_pDeviceProxy->Start()))
		{
			ret = P2P_THREAD_NG;
			P2P_SetLastError(ret);
			return ret;
		}
	}

	if (!SUCCEEDED(m_pPsAgent->Init(m_localAddress, this)))
	{
		ret = P2P_SOCKET_NG;
		P2P_SetLastError(ret);
		return ret;
	}

	if (!SUCCEEDED(m_pP2PConnMgr->Init(this, m_pPsAgent)))
	{
		ret = P2P_SYSTEM_ERROR;
		P2P_SetLastError(ret);
		return ret;
	}

	hr = m_pP2PSocketThread->Init();
	if (!SUCCEEDED(hr))
	{
		ret = P2P_THREAD_NG;
		P2P_SetLastError(ret);
		return ret;
	}
	hr = m_pP2PSocketThread->Start();
	if (!SUCCEEDED(hr))
	{
		ret = P2P_THREAD_NG;
		P2P_SetLastError(ret);
		return ret;
	}

	hr = m_pP2PSocketThread->AddP2PSocket(m_pPsAgent->GetP2PSocket());
	if (!SUCCEEDED(hr))
	{
		ret = P2P_THREAD_NG;
		P2P_SetLastError(ret);
		return ret;
	}

	hr = m_pP2PConnMgr->Start();
	if (!SUCCEEDED(hr))
	{
		ret = P2P_THREAD_NG;
		P2P_SetLastError(ret);
		return ret;
	}

	return ret;
}

void P2PAgentImpl::Stop()
{
	if (m_pP2PSocketThread != NULL)
	{
		m_pP2PSocketThread->SignalForStop(true);
		m_pP2PSocketThread->WaitForStopAndClose();
	}

	if (m_pPsAgent != NULL)
	{
		m_pPsAgent->Close();
	}

	if (m_pP2PConnMgr != NULL)
	{
		m_pP2PConnMgr->Stop();
	}

	if (m_pDeviceProxy != NULL)
	{
		m_pDeviceProxy->Stop();
	}
}

uint32_t P2PAgentImpl::Send2NS(const void *buff, int size, uint32_t destIpHostByteOrder, uint16_t destPort)
{
	if (m_pPsAgent != NULL)
	{
		if (!SUCCEEDED(m_pPsAgent->SendData(buff, size, destIpHostByteOrder, destPort)))
		{
			return P2P_SEND_DATA_NG;
		}
		return P2P_OK;
	}
	return P2P_SYSTEM_ERROR;
}

uint32_t P2PAgentImpl::Send2Peer(const void *buff, int size, const devid_t &destDevId)
{
	if (m_pP2PConnMgr != NULL)
	{
		return m_pP2PConnMgr->Send2Peer(buff, size, destDevId);
	}
	return P2P_SYSTEM_ERROR;
}

uint32_t P2PAgentImpl::GetConnectDevs(devid_list_t &devidList)
{
	if (m_pP2PConnMgr == NULL)
	{
		return P2P_SYSTEM_ERROR;
	}
	devidList.clear();
	
	return m_pP2PConnMgr->GetConnectDevs(devidList);
}

void P2PAgentImpl::HandleNSData(void* buff, int size, uint32_t fromIpHostByteOrder, uint16_t fromPort)
{
	if (m_pP2PHandle != NULL)
	{
		m_pP2PHandle->HandleNSData(buff, size, fromIpHostByteOrder, fromPort);
	}
}

void P2PAgentImpl::HandlePeerData(void* buff, int size, const devid_t& fromDev)
{
	if (m_pDeviceProxy != NULL)
	{
		m_pDeviceProxy->HandlePeerData(buff, size, fromDev);
	}
	else if (m_pP2PHandle != NULL)
	{
		m_pP2PHandle->HandlePeerData(buff, size, fromDev);
	}
}

void P2PAgentImpl::OnConnEvent(event_type_t type, const devid_t& devId)
{
	MyLock lock(&m_lock);
	lock.SetBusy();
	if (m_pP2PHandle != NULL)
	{
		m_pP2PHandle->OnConnEvent(type, devId);
	}
	if (m_pDeviceProxy != NULL)
	{
		if (type == p2p_connected)
		{
			m_pDeviceProxy->UpdatePeerDevId(devId);
		}
		else if (type == p2p_disconnected)
		{
			m_pDeviceProxy->UpdatePeerDevId(INVALID_DEVID);
		}
	}
}

void P2PAgentImpl::HandleSessionData(void* buff, int size, void* obj)
{
	if (m_pP2PHandle != NULL)
	{
		m_pP2PHandle->HandleSessionData(buff, size, obj);
	}
}

void P2PAgentImpl::HandleSessionMediaAttr(const char* sessionId,
		uint32_t audioCodec,
		uint32_t audioSamplerate,
		uint32_t audioChannel)
{
	if (m_pP2PHandle != NULL)
	{
		m_pP2PHandle->HandleSessionMediaAttr(sessionId, audioCodec, audioSamplerate, audioChannel);
	}
}

void P2PAgentImpl::OnSessionEvent(session_event_type_t type, const char* sessionId, int errcode)
{
	if (m_pP2PHandle != NULL)
	{
		m_pP2PHandle->OnSessionEvent(type, sessionId, errcode);
	}
}

P2PSocketThread* P2PAgentImpl::GetP2PSocketThread()
{
	return m_pP2PSocketThread;
}

P2PConnMgr* P2PAgentImpl::GetP2PConnMgr()
{
	return m_pP2PConnMgr;
}

uint32_t P2PAgentImpl::StartSessionRelay(const char* sessionId, const char* url, void* obj)
{
	if (m_pDeviceProxy == NULL)
	{
		return P2P_IS_RELAY_NODE;
	}
	return m_pDeviceProxy->StartSessionRelay(sessionId, url, obj);
}

uint32_t P2PAgentImpl::StopSessionRelay(const char* sessionId)
{
	if (m_pDeviceProxy == NULL)
	{
		return P2P_IS_RELAY_NODE;
	}
	return m_pDeviceProxy->StopSessionRelay(sessionId);
}

