/*************************************************************************
    > File Name: messagereader.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月21日 星期六 10时32分39秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "messagereader.h"
#include "protocal.h"
#include <iostream>

using namespace std;

P2PMessageReader::P2PMessageReader(): m_pProtocalParse(NULL)
{
	Reset();
}

P2PMessageReader::~P2PMessageReader()
{

}

void P2PMessageReader::Reset()
{
	m_state = HeaderNotRead;
	m_cmdType = 0;
	m_checksum = 0;
	m_bodyLength = 0;
	m_stream.Reset();
	m_cmd = "";
	m_subCmd.clear();
	m_subAck.clear();
	if (m_pProtocalParse != NULL)
	{
		delete m_pProtocalParse;
		m_pProtocalParse = NULL;
	}
}

P2PMessageReader::ReaderParseState P2PMessageReader::AddBytes(const uint8_t* pData, uint32_t size)
{
	HRESULT hr = S_OK;
	uint16_t currentSize;

	if (m_state == ParseError)
	{
		return ParseError;
	}
	if (size == 0)
	{
		return m_state;
	}

	// seek to the end of the stream
	m_stream.SeekDirect(m_stream.GetSize());

	if (FAILED(m_stream.Write(pData, size)))
	{
		return ParseError;
	}

	currentSize = m_stream.GetSize();

	if (m_state == HeaderNotRead)
	{
		if (currentSize >= TB_HEAD_SIZE)
		{
			hr = ReadHeader();

			m_state = SUCCEEDED(hr) ? HeaderValidated : ParseError;

			if (SUCCEEDED(hr) && (m_bodyLength == 0))
			{
				m_state = BodyValidated;
			}
		}
	}

	if (m_state == HeaderValidated)
	{
		if (currentSize >= (m_bodyLength + TB_HEAD_SIZE))
		{
			if (currentSize == (m_bodyLength + TB_HEAD_SIZE))
			{
				hr = ReadBody();
				m_state = SUCCEEDED(hr) ? BodyValidated : ParseError;
			}
			else
			{
				// TOO MANY BYTES FED IN
				m_state = ParseError;
			}
		}
	}

	if (m_state == BodyValidated)
	{
		if (currentSize > (m_bodyLength + TB_HEAD_SIZE))
		{
			m_state = ParseError;
		}
	}

	return m_state;
}

HRESULT P2PMessageReader::ReadHeader()
{
	HRESULT hr = S_OK;
	bool fHeaderValid = false;
	uint16_t stag = 0;
	uint16_t version = 0;
	uint8_t checksum = 0;
	uint8_t cmdType = 0;
	uint32_t seqNo = 0;
	uint16_t bodyLength = 0;

	Chk(m_stream.SeekDirect(0));
	Chk(m_stream.ReadUint16(&stag));
	Chk(m_stream.ReadUint16(&version));
	Chk(m_stream.ReadUint8(&checksum));
	Chk(m_stream.ReadUint8(&cmdType));
	Chk(m_stream.ReadUint32(&seqNo));
	Chk(m_stream.ReadUint16(&bodyLength));

	stag = ntohs(stag);
	version = ntohs(version);
	seqNo = ntohl(seqNo);
	bodyLength = ntohs(bodyLength);

	fHeaderValid = (stag == TB_HEAD_STAG);
	ChkIf(!fHeaderValid, E_FAIL);

	m_version = version;
	m_checksum = checksum;
	m_cmdType = cmdType;
	m_seqNo = seqNo;
	m_bodyLength = bodyLength;

Cleanup:
	return hr;
}

HRESULT P2PMessageReader::ReadBody()
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
//	std::cout << "checksum ok" << " CMD Type:" << (int)m_cmdType  << endl;

	if (m_cmdType == P2P_STREAM)
	{
		// p2p stream do not parse body
		return hr;
	}
//	if (IsPsCmdType())
	{
		Chk(ParseBody());
		Chk(ParseProtocal());
	}

//	std::cout << "Parse protocal ok" << endl;

Cleanup:
	return hr;
}

HRESULT P2PMessageReader::ParseBody()
{
	HRESULT hr = S_OK;
	CRefCountedBuffer msgbuff;
	Json::Reader reader;

    hr = m_stream.GetBuffer(&msgbuff);
	if (!SUCCEEDED(hr))
	{
		std::cout << "P2PMessageReader::ParseBody system error" << endl;
		return hr;
	}

	if (strstr((char*)msgbuff->GetData() + TB_HEAD_SIZE, NATTraversalHB) == NULL)
	{
		std::cout << "Parse Msg: " << (char*)msgbuff->GetData() + TB_HEAD_SIZE << endl;
	}
	
	if (!reader.parse((char*)msgbuff->GetData() + TB_HEAD_SIZE, m_bodyValue))
	{
		std::cout << "Parse json data failed:" << (char*)msgbuff->GetData() + TB_HEAD_SIZE << endl;
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
	
	if (m_cmd != PASSTHROUGH_CMD) 
	{
		return hr;
	}

	const Json::Value& dataObj = m_bodyValue["data"];
	m_subCmd = dataObj["subCmd"].asString();
	m_subAck = dataObj["subAck"].asString();

	return hr;
}

HRESULT P2PMessageReader::ParseProtocal()
{
	HRESULT hr = S_OK;

	if (m_pProtocalParse != NULL)
	{
		delete m_pProtocalParse;
		m_pProtocalParse = NULL;
	}
	
	if (m_cmd != PASSTHROUGH_CMD)
	{
		if (m_cmd.compare(P2PStartRelay) == 0
			|| m_cmd.compare(P2PStopRelay) == 0
			|| m_cmd.compare(P2PNotifyStopRelay) == 0)
		{
			m_pProtocalParse = new P2PRelayReqParse();
		}
		else if (m_cmd.compare(ResponseCmd) == 0
			&& (m_retCmd.compare(P2PStartRelay) == 0
			|| m_retCmd.compare(P2PStopRelay) == 0
			|| m_retCmd.compare(P2PNotifyStopRelay) == 0))
		{
			m_pProtocalParse = new P2PRelayRespParse();
		}
		else
		{
			return hr;
		}
	}
	else
	{
		if (m_subCmd.compare(NATTraversalNotify) == 0)
		{
			m_pProtocalParse = new NATTraversalNotifyParse();
		}
		else if (m_subCmd.compare(NATTraversalReq) == 0)
		{
			m_pProtocalParse = new NATTraversalReqParse();
		}
		else if (m_subCmd.compare(NATTraversalResp) == 0)
		{
			m_pProtocalParse = new NATTraversalRespParse();
		}
		else
		{
			m_pProtocalParse = new NATProtocalParseBase();
		}
	}

	ChkIf(m_pProtocalParse == NULL, E_FAIL);

	Chk(m_pProtocalParse->ParseCmd(m_bodyValue));

Cleanup:
	return hr;
}

P2PMessageReader::ReaderParseState P2PMessageReader::GetState()
{
	return m_state;
}

bool P2PMessageReader::IsPsCmdType() const
{
	return (m_cmdType == PS_MSG_TYPE);
}

const char* P2PMessageReader::SubCmd()const
{
	return m_subCmd.c_str();
}

const char* P2PMessageReader::SubAck()const
{
	return m_subAck.c_str();
}

uint8_t P2PMessageReader::CmdType() const
{
	return m_cmdType;
}

const char* P2PMessageReader::Cmd() const
{
	return m_cmd.c_str();
}

const char* P2PMessageReader::Source() const
{
	return m_source.c_str();
}

const char* P2PMessageReader::Target() const
{
	return m_target.c_str();
}

const IProtocalParse* P2PMessageReader::GetNatProtocalParse()const
{
	return m_pProtocalParse;
}

uint32_t P2PMessageReader::SeqNo()const
{
	return m_seqNo;
}

const char* P2PMessageReader::RetCmd() const
{
	return m_retCmd.c_str();
}

const char* P2PMessageReader::RetMsg() const
{
	return m_retMsg.c_str();
}

int P2PMessageReader::RetCode() const
{
	return m_retCode;
}

int P2PMessageReader::BodyLength() const
{
	return m_bodyLength;
}

void P2PMessageReader::Print()
{
	CRefCountedBuffer msgBuf;
	HRESULT hr = m_stream.GetBuffer(&msgBuf);
	if (SUCCEEDED(hr))
	{
		printf("%s\n", (char*)msgBuf->GetData() + TB_HEAD_SIZE);
	}
}

