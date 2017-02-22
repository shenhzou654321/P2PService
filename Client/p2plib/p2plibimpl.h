/*************************************************************************
    > File Name: p2plibimpl.h
    > Author: PYW 
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月20日 星期五 14时18分54秒
 ************************************************************************/

#ifndef _P2PLIBIMPL_H_
#define _P2PLIBIMPL_H_

#include "config.h"
#include "socketaddress.h"
#include "spinlock.h"

class P2PSocketThread;
class PSAgent;
class P2PConnMgr;
class DeviceProxy;

class P2PAgentImpl : public IP2PAgent
{
public:
    P2PAgentImpl(p2p_config_t &config);
	~P2PAgentImpl();

	virtual void RegisterHandle(IP2PHandle *handle);
	virtual void UnRegisterHandle();
	virtual uint32_t Start(uint32_t bindIpHostByteOrder, uint16_t bindPort);
	virtual void Stop();
	virtual uint32_t Send2NS(const void *buff, int size, uint32_t destIpHostByteOrder, uint16_t destPort);
	virtual uint32_t Send2Peer(const void *buff, int size, const devid_t &destDevId);
	virtual uint32_t GetConnectDevs(devid_list_t &devidList);
	virtual uint32_t StartSessionRelay(const char* sessionId, const char* url, void* obj);
	virtual uint32_t StopSessionRelay(const char* sessionId);

	void HandleNSData(void* buff, int size, uint32_t fromIpHostByteOrder, uint16_t fromPort);
	void HandlePeerData(void* buff, int size, const devid_t& fromDev);
	void OnConnEvent(event_type_t type, const devid_t& devId);
	void HandleSessionData(void* buff, int size, void* obj);
	void OnSessionEvent(session_event_type_t type, const char* sessionId, int errcode);
	void HandleSessionMediaAttr(const char* sessionId,
			uint32_t audioCodec,
			uint32_t audioSamplerate,
			uint32_t audioChannel);

	P2PSocketThread* GetP2PSocketThread();
	P2PConnMgr* GetP2PConnMgr();

protected:
	P2PAgentImpl(){;};

	uint32_t Init();
	uint32_t Clear();

private:
	IP2PHandle* m_pP2PHandle;
	p2p_config_t m_p2pconfig;
	P2PSocketThread* m_pP2PSocketThread;
	PSAgent* m_pPsAgent;
	P2PConnMgr* m_pP2PConnMgr;
	DeviceProxy* m_pDeviceProxy;
	CSocketAddress m_localAddress;
	SpinLock m_lock;
};

#endif //_P2PLIBIMPL_H_

