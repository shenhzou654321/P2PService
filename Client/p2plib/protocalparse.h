/*************************************************************************
    > File Name: protocalparse.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月21日 星期六 15时52分30秒
 ************************************************************************/
#ifndef _PROTOCAL_PARSE_H_
#define _PROTOCAL_PARSE_H_

#include "config.h"
#include "protocal.h"
#include <json/json.h>
#include <string>

using std::string;

class IProtocalParse
{
public:
	virtual HRESULT ParseCmd(const Json::Value &bodyValue) = 0;
};

class NATProtocalParseBase : public IProtocalParse
{
protected:
	traversal_id_t m_traversalId;

public:
	virtual HRESULT ParseCmd(const Json::Value &bodyValue);

	const traversal_id_t& TraversalId()const{
		return m_traversalId;
	}

protected:
	HRESULT ParseAddress(const Json::Value &addrValue, string &ip, uint16_t &port);
};

//
// NATTraversalNotify
//

class NATTraversalNotifyParse : public NATProtocalParseBase
{
private:
	devid_t m_targetId;
	string m_stunSvrIpAddr;
	uint16_t m_stunSvrPort;

public:
	virtual HRESULT ParseCmd(const Json::Value &bodyValue);

	devid_t TargetId()const{
		return m_targetId;
	};
	const char* StunSvrIpAddr()const{
		return m_stunSvrIpAddr.c_str();
	};
	uint16_t StunSvrPort()const{
		return m_stunSvrPort;
	};
};

//
// NATTraversalReqParse
//

class NATTraversalReqParse : public NATProtocalParseBase
{
protected:
	devid_t m_devId;
	string m_mappedIpPP;
	uint16_t m_mappedPortPP;
	string m_mappedIpAA;
	uint16_t m_mappedPortAA;
	string m_localIp;
	uint16_t m_localPort;
	string m_stunServerIp;
	uint16_t m_stunServerPort;

public:
	virtual HRESULT ParseCmd(const Json::Value &bodyValue);

	devid_t DevId()const{
		return m_devId;
	}

	const char* MappedIpPP()const{
		return m_mappedIpPP.c_str();
	}

	uint16_t MappedPortPP()const{
		return m_mappedPortPP;
	}

	const char* MappedIpAA()const{
		return m_mappedIpAA.c_str();
	}

	uint16_t MappedPortAA()const{
		return m_mappedPortAA;
	}

	const char* LocalIp()const{
		return m_localIp.c_str();
	}

	uint16_t LocalPort()const{
		return m_localPort;
	}

	const char* StunServerIp()const{
		return m_stunServerIp.c_str();
	}

	uint16_t StunServerPort()const{
		return m_stunServerPort;
	}
};

//
// NATTraversalResp
//

class NATTraversalRespParse : public NATTraversalReqParse
{
	virtual HRESULT ParseCmd(const Json::Value &bodyValue);
};


//
// P2PStartRelay
//

class P2PRelayReqParse : public IProtocalParse
{
public:	
	virtual HRESULT ParseCmd(const Json::Value &bodyValue);
	const char* SessionId() const{
		return m_sessionId.c_str();
	};
	const char* Url() const{
		return m_url.c_str();
	}
	int AudioCodec()const{
		return m_audioCodec;
	}
	int AudioSamplerate()const{
		return m_audioSamplerate;
	}
	int AudioChannel()const{
		return m_audioChannel;
	}

protected:
	string m_sessionId;
	string m_url;
	int m_audioCodec;
	int m_audioSamplerate;
	int m_audioChannel;
};

class P2PRelayRespParse : public IProtocalParse
{
public:
	virtual HRESULT ParseCmd(const Json::Value &bodyValue);

	int SessionHandle() const{
		return m_sessionHandle;
	}

protected:
	int m_sessionHandle;
};

#endif // _PROTOCAL_PARSE_H_


