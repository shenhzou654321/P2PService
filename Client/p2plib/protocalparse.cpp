/*************************************************************************
    > File Name: protocalparse.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月21日 星期六 16时00分01秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "protocalparse.h"
#include <iostream>

using namespace std;

//
// NATProtocalParseBase
//

HRESULT NATProtocalParseBase::ParseCmd(const Json::Value &bodyValue)
{
	string straversalId;
	if (bodyValue[PK_data].isNull())
	{
		std::cout << "NATProtocalParseBase data field is not exist" << endl;
		return E_FAIL;
	}

	const Json::Value& dataObj = bodyValue[PK_data];

	memset(m_traversalId.id, 0, sizeof(m_traversalId.id));
	if (!dataObj[PK_traversalID].isNull())
	{
		straversalId = dataObj[PK_traversalID].asString();
		memcpy(m_traversalId.id, straversalId.c_str(), TRAVERSAL_ID_LENGTH);
	}

	return S_OK;
}

HRESULT NATProtocalParseBase::ParseAddress(const Json::Value &addrValue, 
		string &ip, 
		uint16_t &port)
{
	if (addrValue[PK_ip].isNull()
			|| addrValue[PK_port].isNull())
	{
		std::cout << "Parse address failed" << endl;
		return E_FAIL;	
	}
	ip = addrValue[PK_ip].asString();
	port = addrValue[PK_port].asInt();
	return S_OK;
}

//
// NATTraversalNotifyParse
//

HRESULT NATTraversalNotifyParse::ParseCmd(const Json::Value &bodyValue)
{
	if (bodyValue[PK_data].isNull())
	{
		std::cout << "NATTraversalNotifyParse data field is not exist" << endl;
		return E_FAIL;
	}
	const Json::Value& dataObj = bodyValue[PK_data];

	if (dataObj[PK_traversalID].isNull()
			|| dataObj[PK_targetID].isNull()
			|| dataObj[PK_STUNSvrInfo].isNull())
	{
		std::cout << "NATTraversalNotifyParse key check failed" << endl;
		return E_FAIL;
	}
	string straversalId = dataObj[PK_traversalID].asString();
	memset(m_traversalId.id, 0, sizeof(m_traversalId.id));
	memcpy(m_traversalId.id, straversalId.c_str(), TRAVERSAL_ID_LENGTH);
	m_targetId = (devid_t)dataObj[PK_targetID].asInt();

	const Json::Value& stunSvrInfo = dataObj[PK_STUNSvrInfo];

	if (stunSvrInfo[PK_ipAddr].isNull()
			|| stunSvrInfo[PK_port].isNull())
	{
		std::cout << "NATTraversalNotifyParse STUNSvrInfo field is not exist" << endl;
		return E_FAIL;
	}
	m_stunSvrIpAddr = stunSvrInfo[PK_ipAddr].asString();
	m_stunSvrPort = (uint16_t)stunSvrInfo[PK_port].asInt();

	return S_OK;
}


//
// NATTraversalReqParse
//

HRESULT NATTraversalReqParse::ParseCmd(const Json::Value &bodyValue)
{
	if (bodyValue[PK_data].isNull())
	{
		std::cout << "NATTraversalReqParse data field is not exist" << endl;
		return E_FAIL;
	}

	const Json::Value& dataObj = bodyValue[PK_data];
	if (dataObj[PK_traversalID].isNull()
			|| dataObj[PK_devID].isNull()
			|| dataObj[PK_NATInfo1].isNull()
			|| dataObj[PK_NATInfo2].isNull()
			|| dataObj[PK_localIPInfo].isNull()
			|| dataObj[PK_STUNSvrInfo].isNull())
	{
		std::cout << "NATTraversalReqParse key check failed" << endl;
		return E_FAIL;
	}

	string straversalId = dataObj[PK_traversalID].asString();
	memset(m_traversalId.id, 0, sizeof(m_traversalId.id));
	memcpy(m_traversalId.id, straversalId.c_str(), TRAVERSAL_ID_LENGTH);
	m_devId = (devid_t)dataObj[PK_devID].asInt();

	if (!SUCCEEDED(ParseAddress(dataObj[PK_NATInfo1], m_mappedIpPP, m_mappedPortPP))
		|| !SUCCEEDED(ParseAddress(dataObj[PK_NATInfo2], m_mappedIpAA, m_mappedPortAA))
		|| !SUCCEEDED(ParseAddress(dataObj[PK_localIPInfo], m_localIp, m_localPort))
		|| !SUCCEEDED(ParseAddress(dataObj[PK_STUNSvrInfo], m_stunServerIp, m_stunServerPort)))
	{
		std::cout << "NATTraversalReqParse address parse failed" << endl;
		return E_FAIL;
	}

	return S_OK;
}

//
// NATTraversalRespParse
//

HRESULT NATTraversalRespParse::ParseCmd(const Json::Value &bodyValue)
{
	if (bodyValue[PK_data].isNull())
	{
		std::cout << "NATTraversalRespParse data field is not exist" << endl;
		return E_FAIL;
	}

	const Json::Value& dataObj = bodyValue[PK_data];
	if (dataObj[PK_traversalID].isNull()
			|| dataObj[PK_devID].isNull()
			|| dataObj[PK_NATInfo1].isNull()
			|| dataObj[PK_NATInfo2].isNull()
			|| dataObj[PK_localIPInfo].isNull())
	{
		std::cout << "NATTraversalRespParse key check failed" << endl;
		return E_FAIL;
	}

	string straversalId = dataObj[PK_traversalID].asString();
	memset(m_traversalId.id, 0, sizeof(m_traversalId.id));
	memcpy(m_traversalId.id, straversalId.c_str(), TRAVERSAL_ID_LENGTH);
	m_devId = (devid_t)dataObj[PK_devID].asInt();

	if (!SUCCEEDED(ParseAddress(dataObj[PK_NATInfo1], m_mappedIpPP, m_mappedPortPP))
		|| !SUCCEEDED(ParseAddress(dataObj[PK_NATInfo2], m_mappedIpAA, m_mappedPortAA))
		|| !SUCCEEDED(ParseAddress(dataObj[PK_localIPInfo], m_localIp, m_localPort)))
	{
		std::cout << "NATTraversalRespParse address parse failed" << endl;
		return E_FAIL;
	}

	return S_OK;
}

HRESULT P2PRelayReqParse::ParseCmd(const Json::Value &bodyValue)
{
	if (bodyValue[PK_data].isNull())
	{
		std::cout << "P2PRelayReqParse data field is not exist" << endl;
		return E_FAIL;
	}

	const Json::Value& dataObj = bodyValue[PK_data];

	m_sessionId = dataObj[PK_sessionID].asString();
	m_url = dataObj[PK_url].asString();
	m_audioCodec = dataObj[PK_audioCodec].asInt();
	m_audioSamplerate = dataObj[PK_audioSamplerate].asInt();
	m_audioChannel = dataObj[PK_audioChannel].asInt();
	return S_OK;
}

HRESULT P2PRelayRespParse::ParseCmd(const Json::Value &bodyValue)
{
	if (bodyValue[PK_data].isNull())
	{
		return S_OK;
	}
	const Json::Value& dataObj = bodyValue[PK_data];
	m_sessionHandle = dataObj[PK_sessionHandle].asInt();
	return S_OK;
}


