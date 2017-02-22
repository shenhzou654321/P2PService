/*************************************************************************
    > File Name: protocalbuilder.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月30日 星期一 11时34分02秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "protocalbuilder.h"
#include "protocal.h"
#include <sstream>

using std::stringstream;

ProtocalBuilder::ProtocalBuilder()
{

}

ProtocalBuilder::~ProtocalBuilder()
{
	Reset();
}

HRESULT ProtocalBuilder::AddRequestHead(const char* cmd,
		int cmdType,
		bool isAck,
		const char* source,
		const char* target)
{
	m_Head.clear();
	m_Head["cmd"] = cmd;
	m_Head["cmdType"] = cmdType;
	m_Head["isAck"] = isAck ? 1 : 0;
	m_Head["source"] = source;
	m_Head["target"] = target;

	return S_OK;
}

HRESULT ProtocalBuilder::AddRequestHead(const char* cmd,
		int cmdType,
		bool isAck,
		devid_t source,
		devid_t target)
{
	m_Head.clear();
	m_Head["cmd"] = cmd;
	m_Head["cmdType"] = cmdType;
	m_Head["isAck"] = isAck ? 1 : 0;

	stringstream ssSource;
	stringstream ssTarget;

	ssSource << (int)source;
	ssTarget << (int)target;
	m_Head["source"] = ssSource.str();
	m_Head["target"] = ssTarget.str();

	return S_OK;
}

HRESULT ProtocalBuilder::AddResponseHead(const char* retCmd,
		int cmdType,
		const char* source,
		const char* target,
		int retCode,
		const char* retMsg)
{	
	m_Head.clear();
	m_Head["cmd"] = ResponseCmd;
	m_Head["retCmd"] = retCmd;
	m_Head["cmdType"] = cmdType;
	m_Head["source"] = source;
	m_Head["target"] = target;
	m_Head["retCode"] = retCode;
	m_Head["retMsg"] = retMsg;

	return S_OK;
}

HRESULT ProtocalBuilder::AddDataPram(const char* key,
		const char* value)
{
	m_Data[key] = value;
	return S_OK;
}

HRESULT ProtocalBuilder::AddDataPram(const char* key,
		int value)
{
	m_Data[key] = value;
	return S_OK;
}

HRESULT ProtocalBuilder::AddDataPram(const char* key,
		const char* ip,
		uint16_t port)
{
	Json::Value addr;
	addr["ip"] = ip;
	addr["port"] = port;
	m_Data[key] = addr;
	return S_OK;
}

HRESULT ProtocalBuilder::AddDataPram(const char* key, const CSocketAddress& addr)
{
	Json::Value addrValue;
	char szIp[32];
	uint32_t ip;
	uint16_t port;
	addr.GetIP(&ip, sizeof(ip));
	sprintf(szIp, "%d.%d.%d.%d",
			(ip & 0xFF000000) >> 24,
			(ip & 0x00FF0000) >> 16,
			(ip & 0x0000FF00) >> 8,
			(ip & 0x000000FF));
	port = addr.GetPort();
	addrValue["ip"] = szIp;
	addrValue["port"] = port;
	m_Data[key] = addrValue;
	return S_OK;
}

HRESULT ProtocalBuilder::Reset()
{
	m_Head.clear();
	m_Data.clear();
	return S_OK;
}

const Json::Value& ProtocalBuilder::Commit()
{
	m_Head["data"] = m_Data;
	return m_Head;
}

string Devid2Str(devid_t devid)
{
	stringstream ss;
	ss << (int)devid;
	return ss.str();
}

