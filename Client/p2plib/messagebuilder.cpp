/*************************************************************************
    > File Name: messagebuilder.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月23日 星期一 14时24分01秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "messagebuilder.h"
#include <string>

using std::string;

P2PMessageBuilder::P2PMessageBuilder()
{
	Reset();
}

P2PMessageBuilder::~P2PMessageBuilder()
{
}

void P2PMessageBuilder::Reset()
{
	m_stream.Reset();
}

HRESULT P2PMessageBuilder::GetResult(CRefCountedBuffer* pspBuffer)
{
	return m_stream.GetBuffer(pspBuffer);
}

CDataStream& P2PMessageBuilder::GetStream()
{
	return m_stream;
}

HRESULT P2PMessageBuilder::MakePacket(const void* body,
		uint16_t length,
		uint32_t seqno,
		uint8_t cmdType)
{
	HRESULT hr = S_OK;
	uint8_t checksum = 0;
	const uint8_t* pdata = (uint8_t*)body;

	// calc checksum
	for (int i = 0; i < length; i++)
	{
		checksum += (uint8_t)pdata[i];
	}

	// Add header
	Chk(AddHeader(cmdType, checksum, seqno, length));

	// Add body
	if (length > 0)
	{
		Chk(m_stream.Write(pdata, length));
	}

Cleanup:
	return S_OK;
}

HRESULT P2PMessageBuilder::MakePacket(const Json::Value& bodyValue,
		uint32_t seqno,
		uint8_t cmdType)
{
	Json::FastWriter jsonFastWriter;
	string sbody;
	sbody = jsonFastWriter.write(bodyValue);
	return MakePacket(sbody.c_str(), sbody.length(), seqno, cmdType); 
}

HRESULT P2PMessageBuilder::AddHeader(uint8_t cmdType, 
		uint8_t checksum,
		uint32_t seqno,
		uint16_t length)
{
	HRESULT hr = S_OK;
	uint8_t *pdata = NULL;

	m_stream.Reset();
	Chk(m_stream.SetSizeHint(200));

	Chk(m_stream.WriteUint16(htons(TB_HEAD_STAG)));
	Chk(m_stream.WriteUint16(htons(TB_VERSION)));
	Chk(m_stream.WriteUint8(checksum));
	Chk(m_stream.WriteUint8(cmdType));
	Chk(m_stream.WriteUint32(htonl(seqno)));
	Chk(m_stream.WriteUint16(htons(length)));
	Chk(m_stream.WriteUint32((uint32_t)0));

	// add head checksum
	pdata = m_stream.GetDataPointerUnsafe();
	for (int i=5; i<TB_HEAD_SIZE; i++)
	{
		checksum += *(pdata + i);
	}
	*(pdata + 4) = checksum;

Cleanup:
	return hr;
}




