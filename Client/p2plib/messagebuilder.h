/*************************************************************************
    > File Name: messagebuilder.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月23日 星期一 14时01分58秒
 ************************************************************************/

#ifndef _MESSAGE_BUILDER_H_
#define _MESSAGE_BUILDER_H_

#include <json/json.h>
#include "datastream.h"
#include "protocal.h"

class P2PMessageBuilder
{
private:
	CDataStream m_stream;

private:
	HRESULT AddHeader(uint8_t cmdType, 
			uint8_t checksum,
			uint32_t seqno,
			uint16_t length);

public:
	P2PMessageBuilder();
	~P2PMessageBuilder();

	void Reset();

	HRESULT MakePacket(const Json::Value& bodyValue,
			uint32_t seqno,
			uint8_t cmdType = PS_MSG_TYPE);

	HRESULT MakePacket(const void* body,
			uint16_t length,
			uint32_t seqno,
			uint8_t cmdType = PS_MSG_TYPE);

	HRESULT GetResult(CRefCountedBuffer* pspBuffer);

	CDataStream& GetStream();
};

#endif // _MESSAGE_BUILDER_H_


