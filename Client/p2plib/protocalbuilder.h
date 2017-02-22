/*************************************************************************
    > File Name: protocalbuilder.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月30日 星期一 10时30分24秒
 ************************************************************************/

#ifndef _PROTOCAL_BUILDER_H_
#define _PROTOCAL_BUILDER_H_

#include <json/json.h>
#include <string>
#include "socketaddress.h"
#include "config.h"

using std::string;

class ProtocalBuilder
{
public:
	ProtocalBuilder();
	~ProtocalBuilder();

	HRESULT Reset();

	const Json::Value& Commit();

	HRESULT AddRequestHead(const char* cmd,
			int cmdType,
			bool isAck,
			const char* source,
			const char* target);

	HRESULT AddRequestHead(const char* cmd,
			int cmdType,
			bool isAck,
			devid_t source,
			devid_t target);

	HRESULT AddResponseHead(const char* retCmd,
			int cmdType,
			const char* source,
			const char* target,
			int retCode,
			const char* retMsg
			);

	HRESULT AddDataPram(const char* key, const char* value);
	HRESULT AddDataPram(const char* key, int value);
	HRESULT AddDataPram(const char* key, const char* ip, uint16_t port);
	HRESULT AddDataPram(const char* key, const CSocketAddress& addr);

private:
	Json::Value m_Head;
	Json::Value m_Data;
};

string Devid2Str(devid_t devid);

#endif // _PROTOCAL_BUILDER_H_

