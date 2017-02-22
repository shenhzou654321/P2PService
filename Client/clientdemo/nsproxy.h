/*************************************************************************
    > File Name: nsproxy.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年12月31日 星期四 11时44分14秒
 ************************************************************************/

#ifndef _RELAYNODE_NSPROXY_H_
#define _RELAYNODE_NSPROXY_H_

#include "buffer.h"
#include "nsmsgreader.h"
#include <string>

using std::string;

class NsProxy
{
public:
	enum NsState
	{
		NsDefault,
		NsInit,
		NsWaitRegisterResp,
		NsRegistered,
		NsRegisterNG,
		NsHeartBeatTimeout,
	};
public:
	NsProxy();
	~NsProxy();

	void Init(const char* nsIp, uint16_t nsPort, uint32_t devId, const char* devSn);
	void OnTimer();
	void HandleNsData(void* buff, int size, uint32_t fromIpHostByteOrder, uint16_t fromPort);
	NsState State(){
		return m_state;
	}

protected:
	bool SendRegisterReq();
	bool SendHeartbeatReq();
    uint32_t Send2NS(void* buff, int size);	

	void HandleRegResp(NSMessageReader& msgReader);
	void HandleHBResp(NSMessageReader& msgReader);
	void HandleStartRealPlay(NSMessageReader& msgReader);
	void HandleStopRealPlay(NSMessageReader& msgReader);


private:
	uint32_t m_nsIpHostByteOrder;
	uint16_t m_nsPort;
	string m_nsAddr;
	NsState m_state;
	uint32_t m_heartbeatTick;
	uint32_t m_seqNo;
	CRefCountedBuffer m_spMsgBuf;
	NSMessageReader m_msgReader;
	string m_devId;
	string m_devSn;
	int m_regReqRetry;
	int m_reqTickCount;
    string m_sessionId;
};

#endif // _RELAYNODE_NSPROXY_H_


