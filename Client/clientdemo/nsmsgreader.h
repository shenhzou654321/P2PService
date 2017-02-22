/*************************************************************************
    > File Name: nsmsgreader.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年12月31日 星期四 15时25分10秒
 ************************************************************************/

#ifndef _RElAYNODE_NSMSGREADER_H_
#define _RELAYNODE_NSMSGREADER_H_

#include "messagereader.h"
#include "nsmsgparse.h"
#include <json/json.h>

class NSMessageReader : public P2PMessageReader
{
public:
	

protected:
	virtual HRESULT ReadBody();
	virtual HRESULT ParseProtocal();
	virtual HRESULT ParseBody();
};


#endif // _RELAYNODE_NSMSGREADER_H_


