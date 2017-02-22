
#ifndef _P2P_LIB_H_
#define _P2P_LIB_H_

#include <stdint.h>
#include <vector>

namespace p2plib {

typedef unsigned int devid_t;
typedef std::vector<devid_t> devid_list_t;

struct p2p_config_t
{
    devid_t devid;
};

const devid_t INVALID_DEVID = (devid_t)-1;

enum event_type_t
{
    p2p_connecting,	// p2p连接中
    p2p_failed,		// p2p连接失败
    p2p_connected,	// p2p连接成功
    p2p_disconnected,   // p2p通道已断开
};

enum session_event_type_t
{
	se_start_relay_ok,	// 启动会话中转成功
	se_start_relay_ng,  // 启动会话中转失败
	se_stop_relay_ok,	// 停止会话中转成功
	se_stop_relay_ng,	// 停止会话中转错误
	se_relay_stopped,	// 中转节点停止中转会话
};

//
// IP2PHandle
//

class IP2PHandle
{
public:
	virtual void HandleNSData(void* buff, int size, uint32_t fromIpHostByteOrder, uint16_t fromPort) = 0;
    virtual void HandlePeerData(void* buff, int size, const devid_t &fromDev) = 0;
    virtual void OnConnEvent(event_type_t type, const devid_t &devId) = 0; 
	virtual void HandleSessionData(void* buff, int size, void* obj) = 0;
	virtual void HandleSessionMediaAttr(const char* sessionId,	// 会话ID
			uint32_t audioCodec,								// 音频编码类型 
			uint32_t audioSamplerate,							// 音频采样率
			uint32_t audioChannel) = 0;							// 音频通道数
	virtual void OnSessionEvent(session_event_type_t type, const char* sessionId, int errcode) = 0;
};

//
// IP2PObject
//

class IP2PAgent
{
public:
	virtual void RegisterHandle(IP2PHandle *handle) = 0;
    virtual void UnRegisterHandle() = 0;
    virtual uint32_t Start(uint32_t bindIpHostByteOrder, uint16_t bindPort) = 0;
    virtual void Stop() = 0;
    virtual uint32_t Send2NS(const void* buff, int size, uint32_t destIpHostByteOrder, uint16_t destPort) = 0;
    virtual uint32_t Send2Peer(const void* buff, int size, const devid_t &destDevId) = 0;
    virtual uint32_t GetConnectDevs(devid_list_t &devidList) = 0;
	virtual uint32_t StartSessionRelay(const char* sessionId, const char* url, void* obj) = 0;
	virtual uint32_t StopSessionRelay(const char* sessionId) = 0;
};

}; // namespace p2plib

#ifdef _WIN32
    #ifdef P2PLIB_EXPORTS
    #define P2PLIB_API	__declspec(dllexport)
    #else
    #define P2PLIB_API	__declspec(dllimport)
    #endif
#else
    #define P2PLIB_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

    P2PLIB_API int P2P_Init(const p2plib::p2p_config_t *config, bool isRelayNode = false);
    P2PLIB_API void P2P_Fini();
    P2PLIB_API p2plib::IP2PAgent* P2P_CreateP2PAgent();
    P2PLIB_API void P2P_DestoryP2PAgent(p2plib::IP2PAgent* pP2PAgent);
	P2PLIB_API uint32_t P2P_LastError();
	P2PLIB_API void P2P_SetLastError(uint32_t err);

#ifdef __cplusplus
} // extern "C"
#endif


//
// Error code define
//

const uint32_t P2P_OK = 0;
const uint32_t P2P_SOCKET_NG = 0x80000001;
const uint32_t P2P_THREAD_NG = 0x80000002;
const uint32_t P2P_SEND_DATA_NG = 0x80000003;
const uint32_t P2P_NOT_CONNECTED = 0x80000004;
const uint32_t P2P_LIB_NOT_INIT	= 0x80000005;
const uint32_t P2P_AGENT_CREATED = 0x80000006;
const uint32_t P2P_DATA_PACKET_SIZE_NG = 0x80000007;
const uint32_t P2P_IS_RELAY_NODE = 0x80000008;
const uint32_t P2P_SESSION_ID_ERR = 0x80000009;
const uint32_t P2P_RESP_TIMEOUT = 0x8000000A;
const uint32_t P2P_DISCONNECTED = 0x8000000B;
const uint32_t P2P_SYSTEM_ERROR = 0x8000FFFF;

#endif // _P2P_LIB_H_


