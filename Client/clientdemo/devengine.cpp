/*************************************************************************
    > File Name: devengine.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年12月30日 星期三 16时53分34秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "devengine.h"
#include "config.h"
#include <iostream>

using namespace std;

DevEngine* DevEngine::m_inst = NULL;

DevEngine::DevEngine() : m_p2pAgent(NULL), 
	m_fNeedStop(false), 
	m_pbcsProxy(NULL),
	m_pnsProxy(NULL),
	m_devNo(0),
	m_nsIpHostByteOrder(0),
	m_nsPort(0),
	m_peerDevid(0)
{

}

DevEngine::~DevEngine()
{
	Stop();

	if (m_pbcsProxy != NULL)
	{
		delete m_pbcsProxy;
		m_pbcsProxy = NULL;
	}
	if (m_pnsProxy != NULL)
	{
		delete m_pnsProxy;
		m_pnsProxy = NULL;
	}
}

DevEngine* DevEngine::Instance()
{
	if (m_inst == NULL)
	{
		m_inst = new DevEngine;
	}
	return m_inst;
}

void DevEngine::Destory()
{
	if (m_inst != NULL)
	{
		delete m_inst;
		m_inst = NULL;
	}
}

bool DevEngine::Init()
{
	if (m_pbcsProxy == NULL)
	{
		m_pbcsProxy = new BcsProxy();
	}
	if (m_pnsProxy == NULL)
	{
		m_pnsProxy = new NsProxy();
	}
	return true;
}

bool DevEngine::Start(bool requestNewNs)
{
	if (m_pbcsProxy == NULL)
	{
		return false;
	}

	int ret = 0;
	ret = m_pbcsProxy->Register(requestNewNs);
	if (ret != 0)
	{
		std::cout << "BCS device register failed" << endl;
		return false;
	}

	m_devNo = m_pbcsProxy->DevNo();
	m_secretKey = m_pbcsProxy->SecretKey();
	m_nsAddr = m_pbcsProxy->NsAddr();
	m_nsPort = m_pbcsProxy->NsPort();
	m_nsIpHostByteOrder = ntohl(inet_addr(m_nsAddr.c_str()));

	if (!InitP2PLib())
	{
		return false;
	}

	m_pnsProxy->Init(m_nsAddr.c_str(), m_nsPort, m_devNo, Config::Instance()->DeviceSN());

	Run();

	return true;
}

void DevEngine::Run()
{
	assert(m_pnsProxy != NULL);
	while (!m_fNeedStop)
	{
		m_pnsProxy->OnTimer();

		if (m_pnsProxy->State() == NsProxy::NsHeartBeatTimeout
				|| m_pnsProxy->State() == NsProxy::NsRegisterNG)
		{
			sleep(3);
			Restart();
		}
		// sleep 100 ms
		usleep(100000);
	}
}

bool DevEngine::InitP2PLib()
{
	p2plib::p2p_config_t config;
	config.devid = m_devNo;

	P2P_Init(&config, false);
	m_p2pAgent = P2P_CreateP2PAgent();

	if (m_p2pAgent == NULL)
	{
		std::cout << "Create P2P Agent failed" << endl;
		return false;
	}

	m_p2pAgent->RegisterHandle((IP2PHandle*)this);

	uint32_t ret = m_p2pAgent->Start(0, 20000);
	if (ret != P2P_OK)
	{
		std::cout << "P2P Agent start failed: " << ret << endl;
		return false;
	}

	return true;
}

bool DevEngine::Stop()
{
	m_fNeedStop = true;
	ReleaseP2PLib();

	return true;
}

void DevEngine::ReleaseP2PLib()
{
	if (m_p2pAgent != NULL)
	{
		m_p2pAgent->Stop();
		m_p2pAgent->UnRegisterHandle();
		P2P_DestoryP2PAgent(m_p2pAgent);
		m_p2pAgent = NULL;
		P2P_Fini();
	}
}

bool DevEngine::Restart()
{
	ReleaseP2PLib();

	if (m_pbcsProxy == NULL)
	{
		return false;
	}

	int ret = 0;
	ret = m_pbcsProxy->Register(true);
	if (ret != 0)
	{
		std::cout << "BCS device register failed" << endl;
		return false;
	}

	m_devNo = m_pbcsProxy->DevNo();
	m_secretKey = m_pbcsProxy->SecretKey();
	m_nsAddr = m_pbcsProxy->NsAddr();
	m_nsPort = m_pbcsProxy->NsPort();
	m_nsIpHostByteOrder = ntohl(inet_addr(m_nsAddr.c_str()));

	if (!InitP2PLib())
	{
		return false;
	}

	m_pnsProxy->Init(m_nsAddr.c_str(), m_nsPort, m_devNo, Config::Instance()->DeviceSN());
	return true;
}

uint32_t DevEngine::Send2NS(void* buff, int size)
{
	if (m_p2pAgent == NULL)
	{
		return -1;
	}

	return m_p2pAgent->Send2NS(buff, size, m_nsIpHostByteOrder, m_nsPort);
}


uint32_t DevEngine::Send2Dev(void* buff, int size, devid_t devid)
{
	if (m_p2pAgent == NULL)
	{
		return -1;
	}
	return m_p2pAgent->Send2Peer(buff, size, devid);
}

void DevEngine::HandleNSData(void *buff, int size, uint32_t fromIpHostByteOrder, uint16_t fromPort)
{
//	std::cout << "DevEngine::HandleNSData " << m_pnsProxy << endl;
	if (m_pnsProxy == NULL)
	{
		return;
	}
	m_pnsProxy->HandleNsData(buff, size, fromIpHostByteOrder, fromPort);
}

void DevEngine::HandlePeerData(void *buff, int size, const devid_t &fromDev)
{
}

void DevEngine::OnConnEvent(event_type_t type, const devid_t &devid)
{
	switch (type)
	{
		case p2p_connected:
			std::cout << "Device " << devid << " connected" << endl;
            m_peerDevid = devid;
			break;
		case p2p_disconnected:
			std::cout << "Device " << devid << " disconnected" << endl;
            m_peerDevid = 0;
			break;
		case p2p_connecting:
			std::cout << "Device " << devid << " is conecting" << endl;
			break;
		case p2p_failed:
			std::cout << "Device " << devid << " connect failed" << endl;
			break;
		default:
			break;
	}
}

void DevEngine::HandleSessionData(void *buff, int size, void* obj)
{
//    std::cout << "." << endl;
	uint8_t *pdata = (uint8_t*)buff;
	cout << "recv session data size:" << size << " first 8 bytes:"
		<< std::hex << (int)pdata[0] << " "
		<< std::hex << (int)pdata[1] << " "
		<< std::hex << (int)pdata[2] << " "
		<< std::hex << (int)pdata[3] << " "
		<< std::hex << (int)pdata[4] << " "
		<< std::hex << (int)pdata[5] << " "
		<< std::hex << (int)pdata[6] << " "
		<< std::hex << (int)pdata[7] << endl;
	return;
}

void DevEngine::HandleSessionMediaAttr(const char* sessionId,	// 会话ID
			uint32_t audioCodec,								// 音频编码类型 
			uint32_t audioSamplerate,							// 音频采样率
			uint32_t audioChannel)								// 音频通道数
{
    std::cout << "Audio Attr codec:" << audioCodec << " sample rate:" << audioSamplerate
        << " channel:" << audioChannel << endl;
	return;
}

void DevEngine::OnSessionEvent(session_event_type_t type, const char* sessionId, int errcode)
{
    std::cout << "OnSessionEvent type:" << type << " session id:" << sessionId << " error code:" << errcode << endl;
	return;
}
