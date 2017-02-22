/*************************************************************************
    > File Name: nsmsgparse.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2016年01月04日 星期一 15时06分16秒
 ************************************************************************/

#ifndef _RELAYNODE_NSMSGPARSE_H_
#define _RELAYNODE_NSMSGPARSE_H_

#include "protocalparse.h"


class NSMsgParse : public IProtocalParse
{
public:
	virtual HRESULT ParseCmd(const Json::Value &bodyValue){
		return S_OK;
	};
};

class NSRealPlayReqParse : public IProtocalParse
{
public:
	virtual HRESULT ParseCmd(const Json::Value &bodyValue);
	const char* SessionId() const{
		return m_sessionId.c_str();
	}
	const char* Url() const{
		return m_url.c_str();
	}

protected:
	string m_sessionId;
	string m_url;
};


#endif // _RELAYNODE_NSMSGPARSE_H_


