/*************************************************************************
    > File Name: p2pconn.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月25日 星期三 16时15分30秒
 ************************************************************************/

#ifndef _P2PCONN_H_
#define _P2PCONN_H_

#include "msghandle.h"
#include "p2psocket.h"
#include "protocal.h"
#include "messagereader.h"
#include "messagebuilder.h"
#include "protocalbuilder.h"
#include "stunprocess.h"
#include "oshelper.h"
#include <string>

using std::string;

class PSAgent;
class P2PAgentImpl;

const int P2P_FORCAST_PACKET_COUNT = 1000;
const uint32_t STATE_TIMEOUT = 20000;

class ResendData
{
private:
	int m_retry;
	uint32_t m_lastSendTick;
	CSocketAddress m_destAddr;
	bool m_fToPs;
	CRefCountedBuffer m_buff;
	CDataStream m_stream;
	uint32_t m_seqno;
	string m_cmd;

public:
	ResendData();

	void Reset(CSocketAddress& destAddr,
			CRefCountedBuffer& data,
			uint32_t seqno,
			bool fToPs);

	void Reset(CSocketAddress& destAddr,
			CRefCountedBuffer& data,
			const char* cmd,
			bool fToPs);

	void CheckResp(uint32_t seqno);

	void CheckResp(const char* cmd);

	bool NeedResendData(CSocketAddress& destAddr,
			CRefCountedBuffer& data,
			bool& fToPs);
};

class P2PConn : public IMsgHandle
{
public:
	enum P2PConnState
	{
		InitState,
		WaitBindingTestResult,
		WaitTraversalResponse,
		WaitConnTry2,
		WaitConnAck,
		WaitConnAckResp,
		Connected,
		Timeout,
		Error,
		Stopped,
		Disconnected,
	};

public:
	P2PConn();
	~P2PConn();

	HRESULT Init(const traversal_id_t& traversalId,
			PSAgent* pPSAgent,
			P2PAgentImpl* pP2PAgent);

	void Clear();

	virtual void HandleMsg(CRefCountedBuffer &spBufferIn,
			CSocketAddress &fromAddr,
			CSocketAddress &localAddr,
			int sock);

	void HandleTraversalMsg(P2PMessageReader& msgReader, CSocketAddress& fromAddr);

	uint32_t Send2Peer(const void* buff, int size, bool fIsData=true);
	uint32_t SendCmd(const void* buff, int size, CSocketAddress& addr);

	void OnTick();

	bool IsDead();
	bool IsConnected();
	devid_t DevId(){
		return m_targetDevid;
	}

private:
	void HandleReq(P2PMessageReader& msgReader, CSocketAddress& fromAddr);
	void HandleAck(P2PMessageReader& msgReader, CSocketAddress& fromAddr);

	// request
	void HandleTraversalNotify(P2PMessageReader& msgReader, CSocketAddress& fromAddr);
	void HandleTraversalReq(P2PMessageReader& msgReader, CSocketAddress& fromAddr);
	void HandleTraversalResp(P2PMessageReader& msgReader, CSocketAddress& fromAddr);
	void HandleTraversalTry(P2PMessageReader& msgReader, CSocketAddress& fromAddr);
	void HandleTraversalTry2(P2PMessageReader& msgReader, CSocketAddress& fromAddr);
	void HandleTraversalTestAck(P2PMessageReader& msgReader, CSocketAddress& fromAddr);
	void HandleTraversalHB(P2PMessageReader& msgReader, CSocketAddress& fromAddr);
	void HandleTraversalStop(P2PMessageReader& msgReader, CSocketAddress& fromAddr);

	// ack
	void HandleTraversalReqAck(P2PMessageReader& msgReader, CSocketAddress& fromAddr);
	void HandleTraversalRespAck(P2PMessageReader& msgReader, CSocketAddress& fromAddr);
	void HandleTraversalResuAck(P2PMessageReader& msgReader, CSocketAddress& fromAddr);
	void HandleTraversalTestAckResp(P2PMessageReader& msgReader, CSocketAddress& fromAddr);
	void HandleTraversalHBAck(P2PMessageReader& msgReader, CSocketAddress& fromAddr);
	void HandleNodesTraversalRetryAck(P2PMessageReader& msgReader, CSocketAddress& fromAddr);

	typedef void(P2PConn::*MSG_HANDLER)(P2PMessageReader& msgReader, CSocketAddress& fromAddr);

	bool DispatchRequest(MSG_HANDLER* pfnHandleList,
			const char* cmdList[],
			const char* cmd,
			P2PMessageReader& msgReader,
			CSocketAddress& fromAddr);

	// send request
	void SendNodesTraversalRetry();

	// send traversal result
	void SendTraversalResult(const traversal_id_t& traversalId, int result);

	bool IsTraversaling();
	void UpdateState(P2PConnState state);
	HRESULT ForcastAndSendConnTry();

	HRESULT SendToPS(P2PMessageBuilder &msgBuilder);
	HRESULT SendResponse(uint32_t seqNo,
			const traversal_id_t &traversalId, 
			const char* ackCmd, 
			int code, 
			const char* msg,
			bool fToPS = true,
			const char* newTraversalId = NULL);


	// start traversal
	HRESULT StartTraversal();

	// 请求报文重传处理
	void CheckResendRequest();

	// 更新重传的请求报文
	void UpdateResendRequest(CSocketAddress& destAddr,
		CRefCountedBuffer& data,
		const char* cmd,
		bool fToPs);

	void MasterOnTick();
	void SlaveOnTick();

	void OnMasterWaitBindingTestResult();
	void OnMasterWaitTraversalResponse();
	void OnMasterWaitConnAck();
	void OnMasterConnected();
	void OnSlaveWaitBindingTestResult();
	void OnSlaveWaitConnTry2();
	void OnSlaveConnected();

	void CheckAndSendHB();
	void TimeoutCheck();
	HRESULT ForecastAndSendConnTry();

private:
	P2PSocket m_sock;
    traversal_id_t m_traversalId;
	CSocketAddress m_peerAddr;
	CSocketAddress m_peerMappedAddrPP;
	CSocketAddress m_peerMappedAddrAA;
	CSocketAddress m_peerLocalAddr;
	CSocketAddress m_nsAddr;
	CSocketAddress m_stunServerAddr;
	P2PMessageReader m_msgReader;
	StunProcess m_stunProcess;
	P2PConnState m_state;
	uint32_t m_seq;
	PSAgent* m_pPSAgent;
	P2PAgentImpl* m_pP2PAgent;
	devid_t m_targetDevid;
	uint32_t m_stateTick;
	uint32_t m_hbRecvTick;
	uint32_t m_hbSendTick;
	bool m_fMaster;
	ResendData m_resendData;
	bool m_fReportTestResult;
	CRefCountedBuffer m_spMsgBuf;
};

typedef boost::shared_ptr<P2PConn> RefCountedP2PConn;


#endif // _P2PCONN_H_


