#ifndef __PSUNITS_H__
#define __PSUNITS_H__

#include <vector>
#include "ace/Log_Msg.h"
using std::vector;

const char AbbreviationNameForPS[] = "PS";      //P2PSvr简称
const char AbbreviationNameForBCS[] = "BCS";    //业务控制中心简称
const char AbbreviationNameForNS[] = "NS";      //节点服务器简称
const char AbbreviationNameForSTUN[] = "STUN";      //节点服务器简称

//消息名称
//BCS Command
const char BCSC_RESPONSE[] = "response";        //回应
const char BCSC_GETCONFIG[] = "getConfig";      //获取配置信息
const char BCSC_HEARTBEAT[] = "heartbeat";      //心跳
const char BCSC_GETNODESTOPOLOGYINFO[] = "getNodesTopologyInfo";        //获取节点拓扑结构
const char BCSC_UPDATENODESINFO[] = "updateNodesInfo";
const char BCSC_GETNODEFORPLAY[] = "getNodeForPlay";
const char BCSC_NOTIFY[] = "notify";
//NS Command
const char NSC_DEVSTATUSSYNC[] = "DevStatusSync";               //设备状态同步
const char NSC_DEVSTATUSREPORT[] =  "DevStatusReport";          //设备状态汇报
const char NSC_PASSTHROUGH[] = "PassThrough";                   //中转信息
const char NSC_PT_NODESTRAVERSALRETRY[] = "NodesTraversalRetry";//节点重新穿透
const char NSC_PT_NATTRAVERSALNOTIFY[] = "NATTraversalNotify";  //NAT穿透通知
const char NSC_PT_NATTRAVERSALREQ[] = "NATTraversalReq";        //NAT穿透请求
const char NSC_PT_NATTRAVERSALRESP[] = "NATTraversalResp";      //NAT穿透回应
const char NSC_PT_NATTRAVERSALRESU[] = "NATTraversalResu";      //NAT穿透结果汇报
const char NSC_PT_NATTRAVERSALSTOP[] = "NATTraversalStop";      //NAT穿透停止

//消息关键字
const char KW_TRAVERSAL_ID[] = "traversalID";
const char KW_OLDTRAVERSAL_ID[] = "oldTraversalID";
const char KW_STUNSVR_INFO[] = "STUNSvrInfo";
const char KW_TARGET_ID[] = "targetID";
const char KW_IP_ADDR[] = "ipAddr";
const char KW_PORT[] = "port";
const char KW_TCPPORT[] = "tcpPort";
const char KW_UDPPORT[] = "udpPort";
const char KW_EXTERNAL_ADDR[] = "externalAddr";
const char KW_EXTERNAL_TCPPORT[] = "externalTcpPort";
const char KW_EXTERNAL_UDPPORT[] = "externalUdpPort";
const char KW_SUBCMD[] = "subCmd";
const char KW_SUBACK[] = "subAck";
const char KW_RETCODE[] = "retCode";
const char KW_RETMSG[] = "retMsg";
const char KW_DATA[] = "data";
const char KW_ACTIONEVENT[] = "event";
const char KW_SVR_STARTUP[] = "startup";
const char KW_ACTION[] = "action";
const char KW_DEVID[] = "devID";
const char KW_RESIDUEBAND[] = "residueBand";
const char KW_STATUS[] = "status";
const char KW_RESULT[] = "result";

//状态码
const int RES_CODE_OK = 200;
const int RES_CODE_TS_HB = 300;         //Result Code TraversalSession HeartBeat
const int RES_CODE_TS_FAILED = 400;     //穿透失败
const int RES_CODE_TS_TIMEOUT = 401;    //穿透超时
//状态消息
const char RES_MSG_OK[] = "OK";

//终端状态 Device Status
const char DS_OFFLINE[] = "Offline";

//协议相关
const int PRO_HEAD_SIZE = 16;
const int MSG_BUF_SIZE = 2048; 

const int NS_HB_TIMEOUT = 60;           //单位：S
const int DEV_OFFLINE_TIMEOUT = 300;    //单位：S
const int SCAN_TRM_ONLINE_INTER = 10;   //单位：S
const int SEND_MSG_INTER = 5;           //单位：S，消息重发间隔
const int BCS_HB_INTERVAL = 30;         //单位：S，向BCS发送心跳间隔

typedef int                     DevID_t;
typedef unsigned char           UUID_t[16];
typedef vector<DevID_t>         DevsVector_t;

typedef enum {TS_INIT=1, TS_PREPARE, TS_REQ, TS_RESP, TS_WAIT, TS_HB, TS_END, TS_DIE} TS_Value;
//typedef enum {MSG_REQ=1, MSG_RESP} MSG_TYPE;

#endif
