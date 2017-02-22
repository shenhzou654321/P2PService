#include <iostream>
#include <sstream>
#include "MsgHandler.h"
#include "PSUnits.h"
#include "TCPHandler.h"
#include "TopologyInfo.h"
#include "P2PSvr.h"
#include "TraversalStatus.h"

class P2PSvr;
static MsgHandlerFactory _ins_MsgHandlerFactory;
extern P2PSvr theApp;

MsgHandlerFactory *MsgHandlerFactory::instance()
{
    return &_ins_MsgHandlerFactory;
}

void MsgHandlerFactory::Init()
{
    //"getConfig"
    m_MsgHandlers.insert(std::pair<string, MsgHandler *>(BCSC_GETCONFIG, new MH_GetConfig()));
    //"heartbeat"
    m_MsgHandlers.insert(std::pair<string, MsgHandler *>(BCSC_HEARTBEAT, new MH_HeartBeat()));
    //"getNodesTopologyInfo"
    m_MsgHandlers.insert(std::pair<string, MsgHandler *>(BCSC_GETNODESTOPOLOGYINFO, new MH_GetNodesTopologyInfo()));
    //"updateNodesInfo"
    m_MsgHandlers.insert(std::pair<string, MsgHandler *>(BCSC_UPDATENODESINFO, new MH_UpdateNodesInfo()));
    //"getNodesForPlay"
    m_MsgHandlers.insert(std::pair<string, MsgHandler *>(BCSC_GETNODEFORPLAY, new MH_GetNodesForPlay()));
    //"notify"
    m_MsgHandlers.insert(std::pair<string, MsgHandler *>(BCSC_NOTIFY, new MH_Notify()));
    //"DevStatusSync"
    m_MsgHandlers.insert(std::pair<string, MsgHandler *>(NSC_DEVSTATUSSYNC, new MH_DevStatusSync()));
    //"DevStatusReport"
    m_MsgHandlers.insert(std::pair<string, MsgHandler *>(NSC_DEVSTATUSREPORT, new MH_DevStatusReport()));
    //"PassThrough"
    m_MsgHandlers.insert(std::pair<string, MsgHandler *>(NSC_PASSTHROUGH, new MH_PassThrough()));
    //"PassThrough"--"NodesTraversalRetry"
    m_MsgHandlers.insert(std::pair<string, MsgHandler *>(NSC_PT_NODESTRAVERSALRETRY, new MH_PT_NodesTraversalRetry()));
    //"PassThrough"--"NATTraversalNotify"
    m_MsgHandlers.insert(std::pair<string, MsgHandler *>(NSC_PT_NATTRAVERSALNOTIFY, new MH_PT_NATTraversalNotify()));
    //"PassThrough"--"NATTraversalReq"        
    m_MsgHandlers.insert(std::pair<string, MsgHandler *>(NSC_PT_NATTRAVERSALREQ, new MH_PT_NATTraversalReq()));
    //"PassThrough"--"NATTraversalResp"      
    m_MsgHandlers.insert(std::pair<string, MsgHandler *>(NSC_PT_NATTRAVERSALRESP, new MH_PT_NATTraversalResp()));
    //"PassThrough"--"NATTraversalResu"      
    m_MsgHandlers.insert(std::pair<string, MsgHandler *>(NSC_PT_NATTRAVERSALRESU, new MH_PT_NATTraversalResu()));
}

void MsgHandlerFactory::Fini()
{
    map<string, MsgHandler *>::iterator it = m_MsgHandlers.begin();
    while(it != m_MsgHandlers.end())
    {
        delete it->second;
        m_MsgHandlers.erase(it);
    }
}

MsgHandler *MsgHandlerFactory::GetMsgHandler(const JBody &body)
{
    string cmdStr = body.Cmd();
    if(BCSC_RESPONSE == cmdStr)
    {
        cmdStr = body.RetCmd();
    }

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("Msg[%s][%s]\n"), body.Cmd().c_str(), body.RetCmd().c_str()));
    map<string, MsgHandler *>::iterator it = m_MsgHandlers.find(
            cmdStr.c_str());
    if(it == m_MsgHandlers.end())
    {
        return NULL;
    }
    else
    {
        return it->second;
    }
}

MsgHandler *MsgHandlerFactory::GetMsgHandler(const IData& data)
{
    if(!(data.isMember(KW_SUBCMD)))
    {
        return NULL;
    }

    string subCmd = data[KW_SUBCMD].asString();
    if(BCSC_RESPONSE == subCmd && data.isMember(KW_SUBACK))
    {
        subCmd = data[KW_SUBACK].asString();
    }

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("IData info: subCmd[%s] subAck[%s] dataStr[%s]\n"), data[KW_SUBCMD].asString().c_str(), subCmd.c_str(), data.toStyledString().c_str()));

    map<string, MsgHandler *>::iterator it = m_MsgHandlers.find(subCmd.c_str());
    if(it == m_MsgHandlers.end())
    {
        return NULL;
    }
    else
    {
        return it->second;
    }
}

MsgHandler *MsgHandlerFactory::GetMsgHandler(const char *cmd)
{
    map<string, MsgHandler *>::iterator it = m_MsgHandlers.find(cmd);
    if(it == m_MsgHandlers.end())
    {
        return NULL;
    }
    else
    {
        return it->second;
    }
}

string MsgHandler::ResponseBaseMsg(JBody *body, bool isAck, int retCode, 
        const char *retMsg, IData *data)
{
    JBody resBody;
    resBody.Cmd(BCSC_RESPONSE);
    resBody.RetCmd(body->Cmd());
    resBody.IsACK(isAck);
    resBody.Source(AbbreviationNameForPS);
    resBody.Target(body->Source());
    resBody.RetCode(retCode);
    resBody.RetMsg(retMsg);
    if(data != NULL)
        resBody.Data(*data);
    Head head;
    string buff = head.Serialize(resBody);
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("ResponseBaseMsg End[%d %s]\n"), buff.size(), buff.c_str()));
    return buff;
}

/**
 * @brief HandleMsg 
 *      消息处理<获取配置信息>类
 *
 * @param body 消息体
 * @param handler 接收消息句柄（有些消息需要使用原句柄进行回应)
 *
 * @return 
 *      -1:异常
 *      0:处理正常
 */
int MH_GetConfig::HandleMsg(void *jbodyArgv, 
        ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler)
{
    //todo:body为空异常处理
    JBody *body = (JBody *)jbodyArgv;
    int resCode = body->RetCode();
    //todo: resCode异常
    if(RES_CODE_OK != resCode)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("GetConfig response msg error!\n")), -1);
    }

    IData data = body->Data();

    if(data.isMember(AbbreviationNameForPS))
    {
        IData psInfo = data[AbbreviationNameForPS];         //错误
        string ipAddr = psInfo[0][KW_IP_ADDR].asString();
        int port = psInfo[0][KW_TCPPORT].asInt();
        //ACE_DEBUG((LM_DEBUG, ACE_TEXT("Parse Msg[GetConfig]: ip: %s port: %d\n"), ipAddr.c_str(), port));

        stringstream ss;
        ss << ipAddr << ":" << port;
        string ipInfo = ss.str();
        TCPHandler::instance()->PreProccess(ipInfo);
    }

    if(data.isMember(AbbreviationNameForNS))
    {
        IData nsInfo = data[AbbreviationNameForNS];
        int size = nsInfo.size();
        string ipAddr;
        int port;
        vector<ACE_INET_Addr> addrList;
        for(int i=0; i<size; i++)
        {
            ipAddr = nsInfo[i][KW_EXTERNAL_ADDR].asString();
            port = nsInfo[i][KW_EXTERNAL_TCPPORT].asInt();
            stringstream ss;
            ss << ipAddr << ":" << port;
            string ipInfo = ss.str();
            ACE_INET_Addr addr(ipInfo.c_str());
            addrList.push_back(addr);
        }

        NSClientRepo::instance()->UpdateNSProxys(addrList);
    }

    if(data.isMember(AbbreviationNameForSTUN))
    {
        IData stunInfo = data[AbbreviationNameForSTUN];
        string ipAddr = stunInfo[0][KW_EXTERNAL_ADDR].asString();
        int port = stunInfo[0][KW_EXTERNAL_UDPPORT].asInt();

        theApp.STUNIp(ipAddr);
        theApp.STUNPort(port);
    }

    return 0;
}

string MH_GetConfig::CreateMsg(void *argv)
{
    GetConfigArgv *dataArgv = (GetConfigArgv *)argv;
    JBody body;
    body.Cmd(BCSC_GETCONFIG);
    body.IsACK(true);
    body.Source(AbbreviationNameForPS);
    body.Target(AbbreviationNameForBCS);

    Json::Value data;
    Json::Value item;
    //item["serverType"] = AbbreviationNameForPS;
    //item["queryData"] = "self";
    item["serverType"] = dataArgv->s_ServerType;
    if(dataArgv->s_QueryData == "")
    {
        item["queryData"] = dataArgv->s_QueryData;
    }

    body.Data(item);

    Head head;
    string buff = head.Serialize(body);

    return buff;
}

/**
 * @brief HandleMsg 
 *      消息处理<心跳>类：与BCS心跳
 *
 * @param body 消息体
 * @param handler 接收消息句柄（有些消息需要使用原句柄进行回应)
 *
 * @return 
 *      -1:异常
 *      0:处理正常
 *
 */
int MH_HeartBeat::HandleMsg(void *jbodyArgv, 
        ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler)
{
    JBody *body = (JBody *)jbodyArgv;
    int resCode = body->RetCode();
    if(RES_CODE_OK != resCode)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("HeartBeat response msg error!\n")), -1);
    }
    else
    {
        ACE_ERROR((LM_DEBUG, ACE_TEXT("HeartBeat response msg succ!\n")));
    }

    return 0;
}

string MH_HeartBeat::CreateMsg(void *argv)
{
    JBody body;
    body.Cmd(BCSC_HEARTBEAT);
    body.Source(AbbreviationNameForPS);
    body.Target(AbbreviationNameForBCS);
    Head head;
    string buff = head.Serialize(body);
    return buff;
}

int MH_GetNodesTopologyInfo::HandleMsg(void *jbodyArgv,
        ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler)
{
    JBody *body = (JBody *)jbodyArgv;
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("GetNodesTopologyInfo HandlerMsg\n")));
    int resCode = body->RetCode();
    if(RES_CODE_OK != resCode)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("GetNodesTopologyInfo response msg error!\n")), -1);
    }
    TopologyInfo::FatherSonMap_t map;

    IData data = body->Data();
    IData nodeList = data["nodeList"];
    for(int i=0; i<nodeList.size(); i++)
    {
        DevID_t traversalNode =(DevID_t) nodeList[i]["devNo"].asInt();
        //ACE_DEBUG((LM_DEBUG, ACE_TEXT("GetNodesTopologyInfo FatherDev[%d]\n"), traversalNode));
        IData childrenNodes = nodeList[i]["subDevList"];
        DevsVector_t vector;
        for(int j=0; j<childrenNodes.size(); j++)
        {
            DevID_t tmpDev = childrenNodes[j]["devNo"].asInt();
            vector.push_back(tmpDev);
            //ACE_DEBUG((LM_DEBUG, ACE_TEXT("GetNodesTopologyInfo SonDev[%d] push vector\n"), tmpDev));

        }
        map[traversalNode] = vector;
    }

    TopologyInfo::instance()->FillInTopologyInfo(map);
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("GetNodesTopologyInfo HandlerMsg end\n")));
}

string MH_GetNodesTopologyInfo::CreateMsg(void *argv)
{
    JBody body;
    body.Cmd(BCSC_GETNODESTOPOLOGYINFO);
    body.IsACK(true);
    body.Source(AbbreviationNameForPS);
    body.Target(AbbreviationNameForBCS);

    Head head;
    string buff = head.Serialize(body);
    return buff;
}

int MH_UpdateNodesInfo::HandleMsg(void *jbodyArgv,
        ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler)
{
    JBody *body = (JBody *)jbodyArgv;
    string buff = ResponseBaseMsg(body);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Send Msg: %s\n"), buff.c_str() + 16));

    handler->peer().send(buff.c_str(), buff.size());

    //todo： 改成由事件处理器处理
    //发送获取节点信息请求
    BCSClientProxy::instance()->GetNodesTopologyInfo();

    return 0;
}

int MH_GetNodesForPlay::HandleMsg(void *jbodyArgv,
        ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler)
{
    JBody *body = (JBody *)jbodyArgv;
    DevsVector_t devs;
    IData data = body->Data();
    IData devList = data["devList"];
    uint16_t number = devList.size();
    for(int i=0; i<number; i++)
    {
        DevID_t dev = devList[i]["devNo"].asInt();
        devs.push_back(dev);
    }

    string buff = this->CreateMsg((void *)&devs);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Send Msg: %s\n"), buff.c_str() + 16));

    handler->peer().send(buff.c_str(), buff.size());
    return 0;
}

string MH_GetNodesForPlay::CreateMsg(void *argv)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("GetNodesForPlay start\n")));
    DevsVector_t targets;
    TopologyInfo *ti = TopologyInfo::instance();
    DevsVector_t *devs = (DevsVector_t *)argv;
    DevsVector_t::iterator it = devs->begin();
    while(it != devs->end())
    {
        DevsVector_t tmp = ti->GetTargetDevs(*it);
        targets.insert(targets.begin(), tmp.begin(), tmp.end());
        it++;
    }
    sort(targets.begin(), targets.end());
    targets.erase(unique(targets.begin(), targets.end()), targets.end());

    JBody body;
    body.Cmd(BCSC_RESPONSE);
    body.RetCmd(BCSC_GETNODEFORPLAY);
    body.RetCode(200);
    body.RetMsg("success");
    int i = 0;
    IData devNo = Json::arrayValue;
    DevsVector_t::iterator cit = targets.begin();
    while(cit != targets.end())
    {
        devNo[i]["devNo"] = *cit;
        i++;
        cit++;
    }
    IData devList;
    devList["devList"] = devNo;
    //devList["devList"] = Json::arrayValue;
    body.Data(devList);

    Head head;
    string buff = head.Serialize(body);
    return buff;
}

int MH_Notify::HandleMsg(void *jbodyArgv,
        ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler)
{
    JBody *body = (JBody *)jbodyArgv;
    int resCode = body->RetCode();
    if(RES_CODE_OK != resCode)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("HeartBeat response msg error!\n")), -1);
    }
    else
    {
        ACE_ERROR((LM_DEBUG, ACE_TEXT("HeartBeat response msg succ!\n")));
    }

    return 0;
}

string MH_Notify::CreateMsg(void *argv)
{
    JBody body;
    body.Cmd(BCSC_NOTIFY);
    body.IsACK(true);
    body.Source(AbbreviationNameForPS);
    body.Target(AbbreviationNameForBCS);

    Json::Value data;
    Json::Value item;
    item[KW_ACTIONEVENT] = KW_SVR_STARTUP;
    data[KW_ACTION] = item;

    body.Data(data);

    Head head;
    string buff = head.Serialize(body);

    return buff;
}

/**
 * @brief HandleMsg 
 *      消息处理<设备状态同步>类
 *
 * @param body 消息体
 * @param handler 接收消息句柄（有些消息需要使用原句柄进行回应)
 *
 * @return 
 *      -1:异常
 *      0:处理正常
 */
int MH_DevStatusSync::HandleMsg(void *jbodyArgv, 
        ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler)
{
    ACE_INET_Addr addr;
    handler->peer().get_remote_addr(addr);
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("DevStatusSync: HandleMsg Start\n")));

    JBody *body = (JBody *)jbodyArgv;
    IData data = body->Data();
    IData info = data["infos"];          //数组
    uint16_t number = info.size();
    for(int i=0; i<number; i++)
    {
        int devId = info[i][KW_DEVID].asInt();
        int bandWidth = info[i][KW_RESIDUEBAND].asInt();
        int status = info[i][KW_STATUS].asInt();

        DevObj *dev = DevRepo::instance()->GetDevObj(devId, true);    
        if(dev != NULL)
        {
            dev->DevBandWidth(bandWidth);
            dev->UpdateDevStatus(addr, status);
        }

        //ACE_DEBUG((LM_DEBUG, ACE_TEXT("DevStatusSync: [%d] devId[%ld] reBandWidth[%d]\n"), devId, bandWidth));
    }

    string buff = ResponseBaseMsg(body);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Send Msg: %s\n"), buff.c_str() + 16));

    handler->peer().send(buff.c_str(), buff.size());
    return 0;
}

/**
 * @brief HandleMsg 
 *      消息处理<设备状态汇报>类
 *
 * @param body 消息体
 * @param handler 接收消息句柄（有些消息需要使用原句柄进行回应)
 *
 * @return 
 *      -1:异常
 *      0:处理正常
 *
 */
int MH_DevStatusReport::HandleMsg(void *jbodyArgv, 
        ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler)
{
    JBody *body = (JBody *)jbodyArgv;
    IData data = body->Data();
    int devID = data["devID"].asInt();

    DevObj *dev = DevRepo::instance()->GetDevObj(devID, true);    
    int status = data[KW_STATUS].asInt();
    ACE_INET_Addr addr;
    handler->peer().get_remote_addr(addr);
    if(dev != NULL)
    {
        dev->UpdateDevStatus(addr, status);
    }

    string buff = ResponseBaseMsg(body);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Send Msg: %s\n"), buff.c_str() + 16));

    handler->peer().send(buff.c_str(), buff.size());
    return 0;
}

/**
 * @brief HandleMsg 
 *      消息处理<中转>类
 *
 * @param body 消息体
 * @param handler 接收消息句柄（有些消息需要使用原句柄进行回应)
 *
 * @return 
 *      -1:异常
 *      0:处理正常
 *
 */
int MH_PassThrough::HandleMsg(void *jbodyArgv, 
        ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler)
{
    JBody *body = (JBody *)jbodyArgv;
    int devID = atoi(body->Source().c_str());

    IData data = body->Data();
    //JBody subBody(&data);
    MsgHandler *msgHandler = MsgHandlerFactory::instance()->GetMsgHandler(data);
    if(handler == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Invalid msg cmd!\n")), -1);
    }

    PassThroughArgv argv;
    argv.s_DevID = devID;
    argv.s_SubArgv = (void *)&data;

    msgHandler->HandleMsg((void *)&argv, handler);
    return 0;
}

string MH_PassThrough::CreateMsg(void *argv )
{
    PassThroughArgv *ptArgv = (PassThroughArgv *)argv;

    JBody body;
    body.Cmd(NSC_PASSTHROUGH);
    body.IsACK(false);
    body.Source(AbbreviationNameForPS);
    body.Target(ptArgv->s_DevID);

    string subCmdBody;
    //获取子命令处理器
    MsgHandler *msgHandler = MsgHandlerFactory::instance()->GetMsgHandler(ptArgv->s_Cmd.c_str());
    if(msgHandler == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Invalid msg cmd!\n")), NULL);
    }
    subCmdBody = msgHandler->CreateMsg(ptArgv);

    //格式化字符串（成为Json格式）
    Json::Reader reader;
    Json::Value value;
    reader.parse(subCmdBody, value);

    body.Data(value);

    Head head;
    head.Type(Head::ET_Other);
    string buff = head.Serialize(body);

    return buff;
}

int MH_PT_NATTraversalNotify::HandleMsg(void *argv, 
        ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler)
{
    PassThroughArgv *tmpArgv = (PassThroughArgv *)argv;
    IData *data = (IData *)tmpArgv->s_SubArgv;

    if(!data->isMember(KW_TRAVERSAL_ID)
            || !data->isMember(KW_RETCODE))
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Parse msg[data] failed[NATTraversalNotify]\n")), -1);
    }
    SessionId sid((*data)[KW_TRAVERSAL_ID].asString().c_str());
    TraversalSessionObj *sess = TraversalSessionRepo::instance()->GetSessionObj(sid);
    if(sess == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get session failed[NATTraversalNotify]\n")), -1);
    }

#if 0
    TS_Prepare::TS_PrepareStruct *personStruct = NULL;
    int res = (*data)[KW_RETCODE].asInt();
    //已存在穿透会话
    if(RES_CODE_TS_HB == res && (*data).isMember(KW_OLDTRAVERSAL_ID))
    {
        personStruct = new TS_Prepare::TS_PrepareStruct(res,
                (*data)[KW_OLDTRAVERSAL_ID].asString().c_str());
    }
    else
    {
        personStruct = new TS_Prepare::TS_PrepareStruct(res, "");
    }
#endif

    sess->HandleMsg((void *)data);
}

//构建消息（Response）
string MH_PT_NATTraversalNotify::CreateMsg(void *argv)
{
    PassThroughArgv *pta = (PassThroughArgv *)argv;
    NATTraversalNotify *ntnArgv = (NATTraversalNotify *)pta->s_SubArgv;

    IData dataItem;
    dataItem[KW_SUBCMD] = NSC_PT_NATTRAVERSALNOTIFY;
    dataItem[KW_SUBACK] = BCSC_RESPONSE;

    dataItem[KW_TARGET_ID] = ntnArgv->s_DevID;
    dataItem[KW_TRAVERSAL_ID] = ntnArgv->s_SID.tostr();
    {
        //STUNSvrInfo:
        IData svrInfo;
        svrInfo[KW_IP_ADDR] = ntnArgv->s_STUNIP;
        svrInfo[KW_PORT] = ntnArgv->s_STUNPort;
        dataItem[KW_STUNSVR_INFO] = svrInfo;
    }

    return dataItem.toStyledString();
}

int MH_PT_NATTraversalReq::HandleMsg(void *argv, 
        ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler)
{
    PassThroughArgv *tmpArgv = (PassThroughArgv *)argv;
    DataType type = tmpArgv->s_DataType;

    //消息处理
    IData *data = (IData *)tmpArgv->s_SubArgv;
    DevID_t source = tmpArgv->s_DevID;
    SessionId sid((*data)[KW_TRAVERSAL_ID].asString());
    TraversalSessionObj *sess = TraversalSessionRepo::instance()->GetSessionObj(sid);
    if(sess == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get session failed[NATTraversalNotify]\n")), -1);
    }

    TraversalStatus::BaseData tmpData;
    tmpData.s_Data = *data;
    sess->HandleMsg((void *)&tmpData);

    //构建Respose消息
    if(BCSC_RESPONSE != (*data)[KW_SUBCMD].asString())
    {
        //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalReq CreateRespMsg\n")));
        DevObj *sourceDev = DevRepo::instance()->GetDevObj(source);
        if(sourceDev == NULL)
        {
            ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("[%D](%P|%t) Device[%d] is null(NATTraversalReq)\n"), source), -1);
        }
        NSClientProxy *sourceNSClient = sourceDev->NSProxy();
        if(sourceNSClient == NULL)
        {
            ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get NSService failed[NATTraversalReq]\n")), -1);
        }
        sourceNSClient->NATTraversalReqAck(source, data);
    }
}

//创建NATTraversalReq的Response消息
string MH_PT_NATTraversalReq::CreateMsg(void *argv)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalReq CreateMsg\n")));
    PassThroughArgv *pta = (PassThroughArgv *)argv;
    IData data = *((IData *)pta->s_SubArgv);
    string dataStr;

    switch(pta->s_DataType)
    {
        case REQ:
            {
                dataStr = data.toStyledString();
            }
            break;
        case RESP:
            {
                string subCmd = data[KW_SUBCMD].asString();

                IData msgData;
                msgData[KW_SUBCMD] = BCSC_RESPONSE;
                msgData[KW_SUBACK] = subCmd;
                msgData[KW_TRAVERSAL_ID] = data[KW_TRAVERSAL_ID].asString();
                msgData[KW_RETCODE] = RES_CODE_OK;
                msgData[KW_RETMSG] = RES_MSG_OK;
                dataStr = msgData.toStyledString();
                break;
            }
    }

    return dataStr;
}

int MH_PT_NATTraversalResp::HandleMsg(void *argv, 
        ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalResp\n")));
    PassThroughArgv *tmpArgv = (PassThroughArgv *)argv;
    DataType type = tmpArgv->s_DataType;

    //消息处理
    IData *data = (IData *)tmpArgv->s_SubArgv;
    DevID_t source = tmpArgv->s_DevID;
    SessionId sid((*data)[KW_TRAVERSAL_ID].asString());
    TraversalSessionObj *sess = TraversalSessionRepo::instance()->GetSessionObj(sid);
    if(sess == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get session failed[NATTraversalResp]\n")), -1);
    }

    TraversalStatus::BaseData tmpData;
    tmpData.s_Data = *data;
    sess->HandleMsg((void *)&tmpData);

    //构建Respose消息
    //if(MH_PassThrough::REQ == type)
    if(BCSC_RESPONSE != (*data)[KW_SUBCMD].asString())
    {
        //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalRees CreateRespMsg\n")));
        DevObj *sourceDev = DevRepo::instance()->GetDevObj(source);
        if(sourceDev == NULL)
        {
            ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("[%D](%P|%t) Device[%d] is null(NATTraversalReq)\n"), source), -1);
        }
        NSClientProxy *sourceNSClient = sourceDev->NSProxy();
        if(sourceNSClient == NULL)
        {
            ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get NSService failed[NATTraversalReq]\n")), -1);
        }
        sourceNSClient->NATTraversalRespAck(source, data);
    }
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalReq End\n")));
}

//创建NATTraversalResp的Response消息
string MH_PT_NATTraversalResp::CreateMsg(void *argv)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalResp CreateMsg\n")));
    PassThroughArgv *pta = (PassThroughArgv *)argv;
    IData data = *((IData *)pta->s_SubArgv);
    string dataStr;

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalResp CreateMsg Mid[%d]\n"), pta->s_DataType));
    switch(pta->s_DataType)
    {
        case REQ:
            {
                dataStr = data.toStyledString();
            }
            break;
        case RESP:
            {
                string subCmd = data[KW_SUBCMD].asString();

                IData msgData;
                msgData[KW_SUBCMD] = BCSC_RESPONSE;
                msgData[KW_SUBACK] = subCmd;
                msgData[KW_TRAVERSAL_ID] = data[KW_TRAVERSAL_ID].asString();
                msgData[KW_RETCODE] = RES_CODE_OK;
                msgData[KW_RETMSG] = RES_MSG_OK;
                dataStr = msgData.toStyledString();
                break;
            }
    }

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalResp CreateMsg End[%d %s]\n"), dataStr.size(), dataStr.c_str()));
    return dataStr;
}

int MH_PT_NATTraversalResu::HandleMsg(void *argv, 
        ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalResu\n")));

    PassThroughArgv *pta = (PassThroughArgv *)argv;
    IData data = *((IData *)pta->s_SubArgv);
    DevID_t source = pta->s_DevID;
    SessionId sid(data[KW_TRAVERSAL_ID].asString());
    TraversalSessionObj *sess = TraversalSessionRepo::instance()->GetSessionObj(sid);
    if(sess == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get session failed[NATTraversalNotify]\n")), -1);
    }

    TraversalStatus::BaseData tmpData;
    tmpData.s_Data = data;
    sess->HandleMsg((void *)&tmpData);

    //回应
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalResu CreateRespMsg\n")));
    DevObj *sourceDev = DevRepo::instance()->GetDevObj(source);
    if(sourceDev == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("[%D](%P|%t) Device[%d] is null(NATTraversalReq)\n"), source), -1);
    }
    NSClientProxy *sourceNSClient = sourceDev->NSProxy();
    if(sourceNSClient == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get NSService failed[NATTraversalReq]\n")), -1);
    }
    sourceNSClient->NATTraversalResuAck(argv);
}

string MH_PT_NATTraversalResu::CreateMsg(void *argv)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalResu CreateMsg\n")));
    PassThroughArgv *pta = (PassThroughArgv *)argv;
    IData data = *((IData *)pta->s_SubArgv);

   string subCmd = data[KW_SUBCMD].asString();
   IData msgData;
   msgData[KW_SUBCMD] = BCSC_RESPONSE;
   msgData[KW_SUBACK] = subCmd;
   msgData[KW_TRAVERSAL_ID] = data[KW_TRAVERSAL_ID].asString();
   msgData[KW_RETCODE] = RES_CODE_OK;
   msgData[KW_RETMSG] = RES_MSG_OK;
   string dataStr = msgData.toStyledString();

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalResp CreateMsg End[%d %s]\n"), dataStr.size(), dataStr.c_str()));
    return dataStr;
}

int MH_PT_NodesTraversalRetry::HandleMsg(void *argv, 
        ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalRetry\n")));

    PassThroughArgv *pta = (PassThroughArgv *)argv;
    IData data = *((IData *)pta->s_SubArgv);
    DevID_t source = pta->s_DevID;
    SessionId sid(data[KW_TRAVERSAL_ID].asString());
    TraversalSessionObj *sess = TraversalSessionRepo::instance()->GetSessionObj(sid);
    if(sess == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get session failed[NATTraversalNotify]\n")), -1);
    }

    TraversalStatus::BaseData tmpData;
    tmpData.s_Data = data;
    sess->HandleMsg((void *)&tmpData);

    //回应
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalResu CreateRespMsg\n")));
    DevObj *sourceDev = DevRepo::instance()->GetDevObj(source);
    if(sourceDev == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("[%D](%P|%t) Device[%d] is null(NATTraversalReq)\n"), source), -1);
    }
    NSClientProxy *sourceNSClient = sourceDev->NSProxy();
    if(sourceNSClient == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get NSService failed[NATTraversalReq]\n")), -1);
    }
    sourceNSClient->NATTraversalResuAck(&data);
}

int MH_PT_NATTraversalStop::HandleMsg(void *argv, 
        ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalResu\n")));

    PassThroughArgv *pta = (PassThroughArgv *)argv;
    IData data = *((IData *)pta->s_SubArgv);
    DevID_t source = pta->s_DevID;
    SessionId sid(data[KW_TRAVERSAL_ID].asString());
    TraversalSessionObj *sess = TraversalSessionRepo::instance()->GetSessionObj(sid);
    if(sess == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get session failed[NATTraversalNotify]\n")), -1);
    }

    TraversalStatus::BaseData tmpData;
    tmpData.s_Data = data;
    sess->HandleMsg((void *)&tmpData);
}

string MH_PT_NATTraversalStop::CreateMsg(void *argv)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalStop CreateMsg\n")));
    PassThroughArgv *tmpArgv = (PassThroughArgv *)argv;

    //string subCmd = data[KW_SUBCMD].asString();
    IData msgData;
    msgData[KW_SUBCMD] = NSC_PT_NATTRAVERSALSTOP;
    msgData[KW_SUBACK] = BCSC_RESPONSE;
    msgData[KW_TRAVERSAL_ID] = tmpArgv->s_SidStr;
    string dataStr = msgData.toStyledString();

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("MH_PT_NATTraversalStop CreateMsg End[%d %s]\n"), dataStr.size(), dataStr.c_str()));
    return dataStr;
}
