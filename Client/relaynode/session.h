/*************************************************************************
    > File Name: session.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2016年01月05日 星期二 15时20分52秒
 ************************************************************************/

#ifndef _RELAYNODE_SESSION_H_
#define _RELAYNODE_SESSION_H_

#include "buffer.h"
#include "device.h"
#include "API_PullerModule.h"
#include <string>
#include <vector>

using std::string;
using std::vector;

// 中转数据报文头长度
#define STREAM_HEAD_SIZE	8

// 会话流拉取重试次数最大值
#define PULLER_MAX_RETRY	10

class Session
{
public:
	enum SessionState
	{
		Init,
		Receiving,
		Timeout
	};

	enum StreamType
	{
		st_rtp = 0,
	};

public:
	Session(const char* sessionId,
			const char* url,
			uint32_t sessionHandle);
	~Session();

	void AddDevice(devid_t devid, Device* pdev);
	void RmDevice(devid_t devid);
	bool StartStream();
	void StopStream();
	void ForwdData(void* buff, int size);
	SessionState State(){
		return m_state;
	};
	int DeviceCount(){
		return m_devList.size();
	}

	const char* Id(){
		return m_sessionId.c_str();
	}
	const char* Url(){
		return m_url.c_str();
	}
	uint32_t Handle(){
		return m_sessionHandle;
	}
	bool CheckMagicWord()const;

	void InitMediaAttr(MediaAttr* mediaAttr);

	void OnTimer();

	void RestartStream();

private:
	void DoRestartStream();			// 重新拉取会话
	void NotifySessionStop();		// 通知所有终端会话中转停止
	void NotifyMediaAttr();			// 通知所有终端会话流属性

private:
	string m_sessionId;
	string m_url;
	uint32_t m_sessionHandle;
	typedef vector<devid_t> dev_list_t;
	dev_list_t m_devList;
	SessionState m_state;
	CRefCountedBuffer m_spBuf;
	CRefCountedBuffer m_spMsgBuf;
	char m_magicWord[32];

	bool m_mediaAttrInited;
	MediaAttr m_mediaAttr;
	RTSP_Puller_Handler m_handler;
	uint32_t m_lastPacketTick;
	uint32_t m_restartStreamTick;		// 非0值时表示收到回调返回的错误状态时刻
	uint32_t m_seqno;
	bool m_fStop;
	int m_retryCount;					// 会话流拉取重试次数
};


#endif // _RELAYNODE_SESSION_H_


