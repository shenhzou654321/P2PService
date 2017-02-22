/*************************************************************************
    > File Name: devengine.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年12月30日 星期三 16时53分28秒
 ************************************************************************/

#ifndef _RELAYNODE_DEVENGINE_H_
#define _RELAYNODE_DEVENGINE_H_

#include "p2plib.h"
#include "bcsproxy.h"
#include "nsproxy.h"

using namespace p2plib;

class DevEngine : public IP2PHandle
{
public:
	DevEngine();
	virtual ~DevEngine();

	static DevEngine* Instance();
	static void Destory();

	bool Init();
	bool Start(bool requestNewNs = false);
	bool Stop();

	virtual void HandleNSData(void *buff, int size, uint32_t fromIpHostByteOrder, uint16_t fromPort);
    virtual void HandlePeerData(void *buff, int size, const devid_t &fromDev);
    virtual void OnConnEvent(event_type_t type, const devid_t &devId); 
	virtual void HandleSessionData(void *buff, int size, void* obj);
	virtual void HandleSessionMediaAttr(const char* sessionId,	// 会话ID
			uint32_t audioCodec,								// 音频编码类型 
			uint32_t audioSamplerate,							// 音频采样率
			uint32_t audioChannel);								// 音频通道数
	virtual void OnSessionEvent(session_event_type_t type, const char* sessionId, int errcode);

	uint32_t Send2NS(void* buff, int size);
	uint32_t Send2Dev(void* buff, int size, devid_t devid);
	uint32_t DevNo(){
		return m_devNo;
	};

protected:
	bool InitP2PLib();
	void ReleaseP2PLib();
	bool Restart();		// NS心跳超时后，向BCS请求重新分配NS
	void Run();		// 主线程

private:
	static DevEngine* m_inst;
	IP2PAgent* m_p2pAgent;
	bool m_fNeedStop;
	BcsProxy* m_pbcsProxy;
	NsProxy* m_pnsProxy;
	uint32_t m_devNo;			// 设备ID
	string m_secretKey;		// 设备密钥
	string m_nsAddr;		// NS 服务器地址
	uint32_t m_nsIpHostByteOrder; // NS IP
	unsigned short m_nsPort;	// NS 服务器端口
};


#endif // _RELAYNODE_DEVENGINE_H_



