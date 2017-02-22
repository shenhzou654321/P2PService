#include "commonincludes.hpp"
#include "stunprocess.h"
#include "oshelper.h"
#include "protocal.h"
#include <iostream>

using namespace std;

StunProcess::StunProcess(void) : m_spBufferReader(new CBuffer(MAX_PACKET_LENGTH))
{
	Init(-1);
}

StunProcess::~StunProcess(void)
{
}

void StunProcess::Init(int sock)
{
	m_sock = sock;
	m_lastReqTick = 0;
	m_stunAddrPP = CSocketAddress(0xdddddddd, 10000);
	m_stunAddrAA = CSocketAddress(0xcccccccc, 10000);
	memset(&m_transid, 0, sizeof(m_transid));
	m_state = InitState;
	m_retry = 0;
	m_stunReader.Reset();
}

void StunProcess::SetStunServerAddr(CSocketAddress& stunServerAddr)
{
	m_stunAddrPP = stunServerAddr;
}

HRESULT StunProcess::StartStunTest()
{
	HRESULT hr = S_OK;
	
	ChkIf(m_sock == -1, E_FAIL);

	m_mappedAddrInfo.mappedAddrPP = CSocketAddress(0xaaaaaaaa, 10000);
	m_mappedAddrInfo.mappedAddrAA = CSocketAddress(0xbbbbbbbb, 10000);
	m_mappedAddrInfo.localAddr = CSocketAddress(0xcccccccc, 10000);
	m_mappedAddrInfo.fInitPP = false;
	m_mappedAddrInfo.fInitAA = false;
	m_mappedAddrInfo.updateTickPP = 0;
	m_mappedAddrInfo.updateTickAA = 0;
	m_retry = 0;

	DoBindingTest1();

Cleanup:
	return hr;
}

void StunProcess::HandleMsg(CRefCountedBuffer &spBufferIn,
		CSocketAddress &fromAddr,
		CSocketAddress &localAddr,
		int sock)
{
	CSocketAddress addr;

	m_spBufferReader->SetSize(0);
	m_stunReader.Reset();
	m_stunReader.GetStream().Attach(m_spBufferReader, true);
	m_stunReader.AddBytes(spBufferIn->GetData(), spBufferIn->GetSize());
	if (m_stunReader.GetState() != CStunMessageReader::BodyValidated)
	{
		// wrong packet, throw away
		return;
	}

	m_stunReader.GetXorMappedAddress(&addr);
	// handle stun msg
	if (fromAddr.IsSameIP_and_Port(m_stunAddrPP))
	{
		m_mappedAddrInfo.localAddr = localAddr;
		m_mappedAddrInfo.mappedAddrPP = addr;
		m_mappedAddrInfo.fInitPP = true;
		m_mappedAddrInfo.updateTickPP = GetMillisecondCounter();

		m_stunReader.GetOtherAddress(&m_stunAddrAA);

		DoBindingTest2();
	}
	else
	{
		m_mappedAddrInfo.mappedAddrAA = addr;
		m_mappedAddrInfo.fInitAA = true;
		m_mappedAddrInfo.updateTickAA = GetMillisecondCounter();

		m_state = Completed;
	}

	return;
}

void StunProcess::OnTick()
{
	if (GetMillisecondCounter() - m_lastReqTick < 1000)
	{
		return;
	}
	if (m_retry >= 4)
	{
		m_state = Failed;
		return;
	}
	if (m_state == BindingTest1)
	{
		DoBindingTest1();
		m_retry++;
	}
	if (m_state == BindingTest2)
	{
		DoBindingTest2();
		m_retry++;
	}
}


HRESULT StunProcess::StartBindingRequest(CStunMessageBuilder& builder)
{
	builder.AddBindingRequestHeader();
	builder.AddRandomTransactionId(&m_transid);
	return S_OK;
}


HRESULT StunProcess::DoBindingTest1()
{
	std::cout << "StunProcess::DoBindingTest1" << endl;
	HRESULT hr = S_OK;
	CRefCountedBuffer buffer;
	StunChangeRequestAttribute attribChangeRequest = {};
	CStunMessageBuilder builder;
	Chk(StartBindingRequest(builder));
	builder.AddChangeRequest(attribChangeRequest); 
	builder.FixLengthField();

	Chk(builder.GetStream().GetBuffer(&buffer));
	SendData(buffer->GetData(), buffer->GetSize(), m_stunAddrPP);
	m_lastReqTick = GetMillisecondCounter();
	m_state = BindingTest1;

Cleanup:
	return hr;
}

HRESULT StunProcess::DoBindingTest2()
{
	std::cout << "StunProcess::DoBindingTest2" << endl;
	HRESULT hr = S_OK;
	CRefCountedBuffer buffer;
	StunChangeRequestAttribute attribChangeRequest = {};
	CStunMessageBuilder builder;
	Chk(StartBindingRequest(builder));
	builder.AddChangeRequest(attribChangeRequest); 
	builder.FixLengthField();

	Chk(builder.GetStream().GetBuffer(&buffer));
	SendData(buffer->GetData(), buffer->GetSize(), m_stunAddrAA);
	m_lastReqTick = GetMillisecondCounter();
	m_state = BindingTest2;

Cleanup:
	return hr;
}


int StunProcess::SendData(void* data, int size, CSocketAddress& destAddr)
{
	int sendret = -1;
	if (m_sock == -1)
	{
		return -1;
	}
	sendret = ::sendto(m_sock,
			data,
			size,
			0,
			destAddr.GetSockAddr(),
			destAddr.GetSockAddrLength());
	if (sendret != size)
	{
		return -2;
	}

	return 0;
}

