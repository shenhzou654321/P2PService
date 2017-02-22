/*************************************************************************
    > File Name: device.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2016年01月04日 星期一 16时10分28秒
 ************************************************************************/

#ifndef _RELAYNODE_DEVICE_H_
#define _RELAYNODE_DEVICE_H_

#include "buffer.h"
#include "messagereader.h"
#include "p2plib.h"
#include "resend.h"
#include "API_PullerModule.h"
#include <string>

using namespace p2plib;
using std::string;

class Device
{
public:
	Device(devid_t devid);
	~Device();

	void HandlePeerData(void* buff, int size);
	bool SendNotifyStopRelay(const char* sessionId);
	bool SendMediaAttr(const char* sessionId, MediaAttr* pmediaAttr);
	void OnTimer();
	devid_t DevId(){
		return m_devid;
	}

protected:
	void HandleStartRelayReq(P2PMessageReader& msgReader);
	void HandleStopRelayReq(P2PMessageReader& msgReader);
	void HandleNotifyStopRelayResp(P2PMessageReader& msgReader);

	devid_t m_devid;
	string m_sDevId;
	string m_relaySessionId;
	int m_relaySessionHandle;
	uint32_t m_seqno;

	CRefCountedBuffer m_spMsgBuf;
	P2PMessageReader m_msgReader;
	Resend m_resend;
};

#endif // _RELAYNODE_DEVICE_H
	
