/*************************************************************************
    > File Name: protocal.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年11月20日 星期五 17时11分16秒
 ************************************************************************/

#ifndef _P2P_PROTOCAL_H_
#define _P2P_PROTOCAL_H_

const uint16_t PEER_DATA_STAG = 0xA5A5;
const uint16_t PEER_CMD_STAG = 0xAFAF;
const uint16_t TB_HEAD_STAG = 0xFBFC;
const uint16_t TB_VERSION = 0x01;
const uint16_t TB_HEAD_SIZE = 16;

const uint8_t NS_HEARTBEAT = 0xA0;
const uint8_t PS_MSG_TYPE =	0xB1;
const uint8_t P2P_STREAM = 0xB2;

// 请求报文最大长度
const uint32_t  MAX_PACKET_LENGTH = 1600;

// 请求报文重传间隔 3000ms
const uint32_t REQ_RESEND_INTERVEL = 3000;

// 请求报文最大重传次数
const int REQ_MAX_RESEND_COUNT = 3;

// P2P心跳间隔
const uint32_t PEER_HB_INTERVAL = 15000;

const char PASSTHROUGH_CMD[] = "PassThrough";

const char PS_SOURCE[] = "PS";

#define TRAVERSAL_ID_LENGTH		32

struct traversal_id_t
{
	char id[TRAVERSAL_ID_LENGTH + 2];
	bool operator==(const traversal_id_t& other)
	{
		return (memcmp(id, other.id, TRAVERSAL_ID_LENGTH) == 0);
	}
};

enum msg_type
{
	undefined_msg,  // 终端其它消息
	ps2trm_msg,     // p2p 打通相关的消息
};

const int CMD_TYPE_NORMAL = 1;

const char ResponseCmd[] = "response";

const char NodesTraversalRetry[] = "NodesTraversalRetry";
const char NATTraversalNotify[] = "NATTraversalNotify";
const char NATTraversalReq[] = "NATTraversalReq";
const char NATTraversalResp[] =	"NATTraversalResp";
const char NATTraversalResu[] =	"NATTraversalResu";
const char NATTraversalTest1[] = "NATTraversalTest1";
const char NATTraversalTest2[] = "NATTraversalTest2";
const char NATTraversalTestAck[] = "NATTraversalTestAck";
const char NATTraversalHB[] = "NATTraversalHB";
const char NATTraversalStop[] = "NATTraversalStop";
const char P2PStartRelay[] = "P2PStartRelay";
const char P2PStopRelay[] = "P2PStopRelay";
const char P2PNotifyStopRelay[] = "P2PNotifyStopRelay";
const char P2PSessionMediaAttrNotify[] = "P2PSessionMediaAttrNotify";

// Header
#define PK_cmd					"cmd"
#define PK_retCmd				"retCmd"
#define PK_cmdType				"cmdType"
#define PK_isAck				"isAck"
#define PK_source				"source"
#define PK_target				"target"
#define PK_retCode				"retCode"
#define PK_retMsg				"retMsg"
#define PK_data					"data"

// data
#define PK_subCmd				"subCmd"
#define PK_subACK				"subAck"

// NATTraversalNotify
#define PK_traversalID			"traversalID"
#define PK_targetID				"targetID"
#define PK_STUNSvrInfo			"STUNSvrInfo"
#define PK_ipAddr				"ipAddr"
#define PK_port					"port"
#define PK_oldTraversalID		"oldTraversalID"

// NATTraversalReq
#define PK_devID				"devID"
#define PK_NATInfo1				"NATInfo1"
#define PK_NATInfo2				"NATInfo2"
#define PK_localIPInfo			"localIPInfo"
#define PK_ip					"ip"
#define PK_port					"port"

// NATTraversalResu
#define PK_Result				"result"

// P2PStartRelay,P2PStopRelay,P2PNotifyStopRelay
#define PK_sessionID			"sessionID"
#define PK_url					"url"
#define PK_sessionHandle		"sessionHandle"
#define PK_audioCodec			"audioCodec"
#define PK_audioSamplerate		"audioSamplerate"
#define PK_audioChannel			"audioChannel"

#define ACK_OK					200
#define ACK_CONNECTED			300		// 已穿透
#define ACK_FAIL				501
#define ACK_PULL_STREAM_FAIL	401		// 启动会话流拉取失败
#define ACK_INVALID_SESSION		402		// 会话不存在
#define ACK_TRAVERSAL_FAIL		400		// 穿透失败
#define ACK_TRAVERSAL_TIMEOUT	401	    // 穿透过程中超时


#endif // _P2P_PROTOCAL_H_



