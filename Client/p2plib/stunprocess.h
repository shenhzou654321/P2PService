#pragma once

#include "stunbuilder.h"
#include "stunreader.h"
#include "socketaddress.h"

struct MappedAddrInfo
{
	bool fInitPP;
	bool fInitAA;
	CSocketAddress mappedAddrPP;
	CSocketAddress mappedAddrAA;
	CSocketAddress localAddr;
	uint32_t updateTickPP;
	uint32_t updateTickAA;

	MappedAddrInfo()
	{
		fInitAA = false;
		fInitPP = false;
		updateTickAA = 0;
		updateTickPP = 0;
	}
};

class StunProcess
{
public:
	enum StunProcessState
	{
		InitState,
		BindingTest1,
		BindingTest2,
		Completed,
		Failed,
	};

private:
	MappedAddrInfo m_mappedAddrInfo;
	int m_sock;
	uint32_t m_lastReqTick;
	CSocketAddress m_stunAddrPP;
	CSocketAddress m_stunAddrAA;
	StunTransactionId m_transid;
	int m_retry;
	StunProcessState m_state;
	CStunMessageReader m_stunReader;
	CRefCountedBuffer m_spBufferReader;

public:
	StunProcess(void);
	virtual ~StunProcess(void);

	void Init(int sock);
	void SetStunServerAddr(CSocketAddress& stunServerAddr);

	HRESULT StartStunTest();

	void HandleMsg(CRefCountedBuffer &spBufferIn,
			CSocketAddress &fromAddr,
			CSocketAddress &localAddr,
			int sock);

	virtual void OnTick();

	StunProcessState State(){return m_state;};

	const MappedAddrInfo& GetMappedAddrInfo(){return m_mappedAddrInfo;};

private:
	HRESULT StartBindingRequest(CStunMessageBuilder& builder);

	HRESULT DoBindingTest1();
	HRESULT DoBindingTest2();

	int SendData(void* data, int size, CSocketAddress& destAddr);
};



