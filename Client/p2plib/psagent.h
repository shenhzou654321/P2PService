/*************************************************************************
    > File Name: psagent.h
    > Author: PYW
	> Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月20日 星期五 14时58分38秒
 ************************************************************************/

#ifndef _PSAGENT_H_
#define _PSAGENT_H_

#include "config.h"
#include "msghandle.h"
#include "p2psocket.h"
#include "messagereader.h"

class P2PAgentImpl;

class PSAgent : public IMsgHandle
{
public:
	PSAgent();
	~PSAgent();

	virtual void HandleMsg(CRefCountedBuffer &spBufferIn,
			CSocketAddress &fromAddr,
			CSocketAddress &localAddr,
			int sock);

	HRESULT Init(CSocketAddress &bindAddr, P2PAgentImpl* pP2PAgentImpl);

	void Close();

	P2PSocket* GetP2PSocket();

	HRESULT SendData(const void *buff, 
			int size, 
			uint32_t destIpHostByteOrder,
			uint16_t destPort);

private:
	void HandleP2PCmd(CSocketAddress &fromAddr);

	// 消息处理函数
	typedef void(PSAgent::*MSG_HANDLER)(CSocketAddress &fromAddr);

private:
	P2PSocket m_sock;
	P2PAgentImpl* m_pP2PAgentImpl;
	P2PMessageReader m_msgReader;
	CSocketAddress m_nsAddress;
};

#endif //_PSAGENT_H_

