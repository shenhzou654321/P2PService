/*************************************************************************
    > File Name: p2plibtest.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月26日 星期四 16时00分00秒
 ************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "p2plib.h"

using std::string;
using namespace p2plib;

string value_to_ip(uint32_t ip)
{
	char strIp[20];
	sprintf(strIp, "%d.%d.%d.%d",
			(ip & 0xFF000000) >> 24,
			(ip & 0x00FF0000) >> 16,
			(ip & 0x0000FF00) >> 8,
			(ip & 0x000000FF));
	return string(strIp);
}

class P2PHandleImpl : public IP2PHandle
{
public:
	virtual void HandleNSData(void *buff, int size, uint32_t fromIpHostByteOrder, uint16_t fromPort){
		printf("recv %d bytes from %s:%d\n", size, value_to_ip(fromIpHostByteOrder).c_str(), fromPort);
	};

	virtual void HandlePeerData(void *buff, int size, const devid_t &fromDev){
		printf("recv %d bytes from dev: %d\n", size, fromDev);
	};

	virtual void OnConnEvent(event_type_t type, const devid_t &devId){
		printf("OnConnEvent: type:%d, devid:%d\n", type, devId);
	};
	virtual void HandleSessionData(void* buff, int size, void* obj){
		printf("HandleSessionData %d\n", size);
	};
	virtual void HandleSessionMediaAttr(const char* sessionId,	// 会话ID
			uint32_t audioCodec,								// 音频编码类型 
			uint32_t audioSamplerate,							// 音频采样率
			uint32_t audioChannel)							// 音频通道数
	{
		printf("HandleSessionMediaAttr codec:%d, sample rate:%d, channel:%d\n",
				audioCodec, audioSamplerate, audioChannel);
	};
	virtual void OnSessionEvent(session_event_type_t type, const char* sessionId, int errcode)
	{
		printf("OnSessionEvent: type:%d, sessionId:%s, errcode:%d\n",
				type, sessionId, errcode);
	};
};

void Usage()
{
	printf("Usage: p2plibtest ip [port]\n");
}

int main(int argc, char** argv)
{
	P2PHandleImpl p2phandle;
	p2plib::p2p_config_t config;
	config.devid = 1;
	uint32_t ip;
	uint16_t port = 9988;
	string sDest;

	if (argc <= 1)
	{
		Usage();
		return -1;
	}

	if (argc >= 2)
	{
		ip = ntohl(inet_addr(argv[1]));
		sDest = argv[1];
	}
	if (argc >= 3)
	{
		port = (uint16_t)atoi(argv[2]);
		sDest += ":";
		sDest += argv[2];
	}
	else
	{
		sDest += ":9988";
	}

	P2P_Init(&config);

	p2plib::IP2PAgent* pAgent = P2P_CreateP2PAgent();

	if (pAgent == NULL)
	{
		printf("P2P_CreateP2PAgent failed\n");
		return -1;
	}

	pAgent->RegisterHandle((IP2PHandle*)&p2phandle);

	uint32_t ret = pAgent->Start(0, 9999);
	if (ret != P2P_OK)
	{
		printf("P2PAgent->Start failed: %d\n", ret);
		return -1;
	}

	// send data test
	uint8_t buff[1024];
	memset(buff, 5, sizeof(buff));
	for (int i=0; i<100; i++)
	{
		ret = pAgent->Send2NS(buff, sizeof(buff), ip, port);

		printf("Send %d bytes to %s, ret:0x%08X\n", (int)sizeof(buff), sDest.c_str(), (int)ret);

		sleep(1);
	}

	pAgent->Stop();
	pAgent->UnRegisterHandle();

	P2P_Fini();

	return 0;
}


