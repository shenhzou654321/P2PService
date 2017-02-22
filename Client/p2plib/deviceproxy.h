/*************************************************************************
    > File Name: deviceproxy.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年12月28日 星期一 16时22分56秒
 ************************************************************************/

#ifndef _DEVICE_PROXY_H_
#define _DEVICE_PROXY_H_

#include "protocal.h"
#include "messagereader.h"
#include "messagebuilder.h"
#include "protocalbuilder.h"
#include <string>

using std::string;

class P2PAgentImpl;

class DeviceProxy
{
public:
	DeviceProxy();
	~DeviceProxy();

	HRESULT Init(P2PAgentImpl* pP2PAgent);
	HRESULT Start();
	void Stop();

	void HandlePeerData(void* buff, int size, const devid_t& fromDev);
	uint32_t StartSessionRelay(const char* sessionId, const char* url, void* userContext);
	uint32_t StopSessionRelay(const char* sessionId);
	void UpdatePeerDevId(devid_t devId);

private:
	void OnTimer();
	void Run();
    static void* ThreadFunction(void* pThis);	
	void HandleStartRelayResp(P2PMessageReader& msgReader, const devid_t& fromDev);
	void HandleStopRelayResp(P2PMessageReader& msgReader, const devid_t& fromDev);
	void HandleRelayStopNotify(P2PMessageReader& msgReader, const devid_t& fromDev);
	void HandleSessionMediaAttrNotify(P2PMessageReader& msgReader, const devid_t& fromDev);

private:
	P2PAgentImpl* m_pP2PAgent;
	string m_sessionId;
	string m_url;
	void* m_userContext;
	uint32_t m_sessionHandle;
	CRefCountedBuffer m_spMsgBuf;
	P2PMessageReader m_msgReader;
	uint32_t m_seq;

	enum RelayState
	{
		RS_IDEL,
		RS_WaitStartResp,
		RS_Relaying,
		RS_WaitStopResp
	};
	RelayState m_relayState;
	int m_retry;
	uint32_t m_lastReqTick;
	CRefCountedBuffer m_spResendMsgBuf;
	devid_t m_peerDevId;
	pthread_t m_pthread;
	bool m_fNeedToExit;
	bool m_fThreadIsValid;
};

#endif // _DEVICE_PROXY_H

