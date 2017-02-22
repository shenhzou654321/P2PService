/*************************************************************************
    > File Name: messagereader.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月20日 星期五 17时07分38秒
 ************************************************************************/

#ifndef _MESSAGE_READER_H_
#define _MESSAGE_READER_H_

#include <json/json.h>
#include <string>
#include "datastream.h"
#include "socketaddress.h"
#include "protocalparse.h"

//using std::string;

class P2PMessageReader
{
public:
	enum ReaderParseState
	{
		HeaderNotRead,
		HeaderValidated,
		BodyValidated,
		ParseError
	};

protected:
	CDataStream m_stream;
	ReaderParseState m_state;
    uint8_t m_cmdType;
	uint8_t m_checksum;
	uint32_t m_seqNo;
	uint16_t m_version;
	uint16_t m_bodyLength;
	string m_cmd;
	string m_retCmd;
	bool m_isAck;
	string m_source;
	string m_target;
	int m_retCode;
	string m_retMsg;
	string m_subCmd;
	string m_subAck;
	Json::Value m_bodyValue;
	IProtocalParse* m_pProtocalParse;

protected:
	virtual HRESULT ReadHeader();
	virtual HRESULT ReadBody();
	virtual HRESULT ParseBody();
	virtual HRESULT ParseProtocal();

public:
	P2PMessageReader();
	virtual ~P2PMessageReader();

	void Reset();

	ReaderParseState AddBytes(const uint8_t* pData, uint32_t size);
	ReaderParseState GetState();

	bool IsPsCmdType() const;
	const char* SubCmd() const;
	const char* SubAck() const;
	uint8_t CmdType() const;
	const char* Cmd() const;
	const char* Source() const;
	const char* Target() const;
	uint32_t SeqNo() const;
	const char* RetCmd() const;
	const char* RetMsg() const;
	int RetCode() const;
	int BodyLength() const;

    const IProtocalParse* GetNatProtocalParse()const;

	void Print();

	CDataStream& GetStream();
};


#endif // _MESSAGE_READER_H_



