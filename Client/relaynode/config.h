/*************************************************************************
    > File Name: config.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年12月30日 星期三 16时33分16秒
 ************************************************************************/

#ifndef _RELAYNODE_CONFIG_H_
#define _RELAYNODE_CONFIG_H_

#include <string>

using std::string;


class Config
{
public:
	Config();
	static Config* Instance();
	bool Load(const char* configfile);

	const char* DeviceSN()
	{
		return m_deviceSN.c_str();
	};
	const char* DeviceSw()
	{
		return m_deviceSw.c_str();
	}
	const char* ApiKey()
	{
		return m_apiKey.c_str();
	};
	const char* PrivateKey()
	{
		return m_privateKey.c_str();
	};
	const char* BCSUri()
	{
		return m_bcsUri.c_str();
	};
	unsigned short BCSPort()
	{
		return m_bcsPort; 
	};
	int HeartBeatInterval()
	{
		return m_heartBeatInterval;
	};
	const char* ProductType()
	{
		return m_productType.c_str();
	};
	int ReqMaxRetry()
	{
		return m_reqMaxRetry;
	};

private:
	string m_deviceSN;
	string m_deviceSw;
	string m_apiKey;
	string m_privateKey;
	string m_bcsUri;
	unsigned short m_bcsPort;
	string m_productType;
	int m_heartBeatInterval;
	int m_reqMaxRetry;
};


#endif // _RELAYNODE_CONFIG_H_

