/*************************************************************************
    > File Name: nsmsgreader.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年12月31日 星期四 15时25分19秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "nsmsgreader.h"
#include <iostream>

using namespace std;

HRESULT NSMessageReader::ParseProtocal()
{
	HRESULT hr = S_OK;

	if (m_pProtocalParse != NULL)
	{
		delete m_pProtocalParse;
		m_pProtocalParse = NULL;
	}
	
	if (m_cmd == ResponseCmd)
	{
		if (m_retCmd == "DeviceReg"
			|| m_retCmd == "DeviceHB")
		{
			m_pProtocalParse = new NSMsgParse();
		}
	}
	else if (m_cmd == "StartRealPlay")
	{
		m_pProtocalParse = new NSRealPlayReqParse();
	}
	else if (m_cmd == "StopRealPlay")
	{
		m_pProtocalParse = new NSRealPlayReqParse();
	}

	ChkIf(m_pProtocalParse == NULL, E_FAIL);
	Chk(m_pProtocalParse->ParseCmd(m_bodyValue));

Cleanup:
	return hr;
}

HRESULT NSMessageReader::ReadBody()
{
	HRESULT hr = S_OK;
	uint8_t checksum = 0;
	uint8_t v = 0;

	Chk(m_stream.SeekDirect(5));
	for (int k=0; k<TB_HEAD_SIZE - 5; k++)
	{
		Chk(m_stream.ReadUint8(&v));
		checksum += v;
	}

	Chk(m_stream.SeekDirect(TB_HEAD_SIZE));

	for (int i=0; i<m_bodyLength; i++)
	{
		Chk(m_stream.ReadUint8(&v));
		checksum += v;
	}

//	std::cout << "check checksum " << (int)m_checksum << " " << (int)checksum << endl;
	ChkIf(m_checksum != checksum, E_FAIL);
//	std::cout << "checksum ok" << endl;

	Chk(ParseBody());
	Chk(ParseProtocal());

Cleanup:
	return hr;
}

HRESULT NSMessageReader::ParseBody()
{
	HRESULT hr = S_OK;
	CRefCountedBuffer msgbuff;
	Json::Reader reader;

    hr = m_stream.GetBuffer(&msgbuff);
	if (!SUCCEEDED(hr))
	{
		return hr;
	}
	
	std::cout << (char*)msgbuff->GetData() + TB_HEAD_SIZE;
	if (!reader.parse((char*)msgbuff->GetData() + TB_HEAD_SIZE, m_bodyValue))
	{
		std::cout << "Json parse failed" << endl;
		return E_FAIL;
	}
	m_cmd = m_bodyValue[PK_cmd].asString();
	m_source = m_bodyValue[PK_source].asString();
	m_target = m_bodyValue[PK_target].asString();
	if (!m_bodyValue[PK_isAck].isNull())
	{
		m_isAck = (m_bodyValue[PK_isAck].asInt() == 1);
	}
	if (m_cmd.compare(ResponseCmd) == 0)
	{
		m_retCmd = m_bodyValue[PK_retCmd].asString();
		m_retCode = m_bodyValue[PK_retCode].asInt();
		m_retMsg = m_bodyValue[PK_retMsg].asString();
	}

	return hr;
}





