/*************************************************************************
    > File Name: bcsproxy.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年12月30日 星期三 17时22分57秒
 ************************************************************************/

#ifndef _RELAYNODE_BCSPROXY_H_
#define _RELAYNODE_BCSPROXY_H_

#include <map>
#include <string>

using std::map;
using std::string;

class BcsProxy
{
public:
	BcsProxy();
	~BcsProxy();

	int Register(bool requestNewNs);
	uint32_t DevNo(){
		return m_devNo;
	}
	const char* SecretKey(){
		return m_secretKey.c_str();
	}
	const char* NsAddr(){
		return m_nsAddr.c_str();
	}
	unsigned short NsPort(){
		return m_nsPort;
	}

protected:
	int DoRegister();
	string GetApiSign();
	int ParseBody(string& body);

private:
	typedef std::map<string, string> param_map_t;
	param_map_t m_registerParams;
	uint32_t m_devNo;			// 设备ID
	string m_secretKey;		// 设备密钥
	string m_nsAddr;		// NS 服务器地址
	unsigned short m_nsPort;	// NS 服务器端口
};

#endif // _RELAYNODE_BCSPROXY_H_


