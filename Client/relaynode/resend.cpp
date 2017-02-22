/*************************************************************************
    > File Name: resend.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2016年01月05日 星期二 13时40分49秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "resend.h"

Resend::Resend() : 
	m_retry(0),
	m_lastSendTick(0),
	m_spBuf(new CBuffer(MAX_PACKET_LENGTH)),
	m_seqno(0)
{

}


