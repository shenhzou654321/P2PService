/*************************************************************************
    > File Name: nsmsgparse.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2016年01月04日 星期一 15时08分33秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "nsmsgparse.h"


HRESULT NSRealPlayReqParse::ParseCmd(const Json::Value &bodyValue)
{
	if (bodyValue[PK_data].isNull())
	{
		return E_FAIL;
	}

	const Json::Value& dataObj = bodyValue[PK_data];

	m_sessionId = dataObj[PK_sessionID].asString();
	m_url = dataObj[PK_url].asString();
	return S_OK;
}



