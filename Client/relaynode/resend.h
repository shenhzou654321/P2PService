/*************************************************************************
    > File Name: resend.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2016年01月05日 星期二 13时33分35秒
 ************************************************************************/

#ifndef _RELAYNODE_RESEND_H_
#define _RELAYNODE_RESEND_H_

#include "buffer.h"
#include "socketaddress.h"
#include "config.h"
#include "protocal.h"
#include "oshelper.h"

class Resend
{
public:
	Resend();

	void SetData(CRefCountedBuffer& data,
			uint32_t seqno,
			CSocketAddress& destAddr);
	void SetData(CRefCountedBuffer& data,
			uint32_t seqno);
	void CheckResp(uint32_t seqno);
	bool GetData(CSocketAddress& destAddr,
			CRefCountedBuffer& data);
	bool GetData(CRefCountedBuffer& data);

private:
	int m_retry;
	uint32_t m_lastSendTick;
	CSocketAddress m_destAddr;
	CRefCountedBuffer m_spBuf;
	uint32_t m_seqno;
	int m_maxRetry;
	int m_interval;
};

inline void Resend::CheckResp(uint32_t seqno)
{
	if (seqno == m_seqno)
	{
		m_seqno = 0;
	}
}

inline void Resend::SetData(CRefCountedBuffer& data,
		uint32_t seqno,
		CSocketAddress& destAddr)
{
	if (data->GetSize() > MAX_PACKET_LENGTH)
	{
		return;
	}
	memcpy(m_spBuf->GetData(), data->GetData(), data->GetSize());
	m_spBuf->SetSize(data->GetSize());
	m_seqno = seqno;
	m_destAddr = destAddr;
	m_retry = 0;
	m_lastSendTick = GetMillisecondCounter();
}

inline void Resend::SetData(CRefCountedBuffer& data,
		uint32_t seqno)
{
	if (data->GetSize() > MAX_PACKET_LENGTH)
	{
		return;
	}
	memcpy(m_spBuf->GetData(), data->GetData(), data->GetSize());
	m_spBuf->SetSize(data->GetSize());
	m_seqno = seqno;
	m_retry = 0;
	m_lastSendTick = GetMillisecondCounter();
}

inline bool Resend::GetData(CSocketAddress& destAddr,
		CRefCountedBuffer& data)
{
	if (m_seqno == 0)
	{
		return false;
	}
	uint32_t tick = GetMillisecondCounter();

	if (m_retry >= m_maxRetry 
		|| tick - m_lastSendTick < (uint32_t)m_interval)
	{
		return false;
	}

	m_retry++;
	m_lastSendTick = GetMillisecondCounter();
	destAddr = m_destAddr;
	data = m_spBuf;

	return true;
}

inline bool Resend::GetData(CRefCountedBuffer& data)
{
	if (m_seqno == 0)
	{
		return false;
	}
	uint32_t tick = GetMillisecondCounter();
	
	if (m_retry >= m_maxRetry 
		|| tick - m_lastSendTick < (uint32_t)m_interval)
	{
		return false;
	}

	m_retry++;
	m_lastSendTick = GetMillisecondCounter();
	data = m_spBuf;

	return true;
}

#endif // _RELAYNODE_RESEND_H_


