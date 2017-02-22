#include "ProtocolLib/JBody.h"
#include "ProtocolLib/Head.h"
#include "ClientProxy.h"
#include "Device.h"
#include "MsgHandler.h"

ClientHandler *NSClientProxy::GenConn()
{
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) NSClientProxy GenConn\n")));

    ClientHandler *handler = new ClientHandler();//NULL;
    if(m_Client.connect(handler, m_IPAddr) == -1)
    {
        ACE_ERROR((LM_ERROR, ACE_TEXT("[%D](%P|%t) Connection failed\n")));
        return NULL;
    }
    return handler;
}

int NSClientProxy::NATTraversalNotify(const DevID_t& targetId, const string& ip, const int port, DevID_t traversalTargetId,const  SessionId& sid)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) NATTraversalNotify Start\n")));

    //BCSClientHandler *handler = (BCSClientHandler *)this->GenConn();
    ClientHandler *handler = this->GenConn();//NULL;

    MsgHandler *msgHandler = MsgHandlerFactory::instance()->GetMsgHandler(NSC_PASSTHROUGH);
    if(msgHandler == NULL)
    {
        return -1;
    }

    //NATTraversalNotify argv
    MH_PT_NATTraversalNotify::NATTraversalNotify notifyArgv(sid, traversalTargetId, ip, port);
    //PassThrough argv
    PassThroughArgv argvPT;
    argvPT.s_DataType = REQ;
    argvPT.s_Cmd = NSC_PT_NATTRAVERSALNOTIFY;
    argvPT.s_DevID = targetId;
    argvPT.s_SubArgv = (void *)&notifyArgv;
    string buff = msgHandler->CreateMsg((void *)&argvPT);

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) Msg: %s\n"), buff.c_str()+16));
    handler->SendMsg(buff.c_str(), buff.size());

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) NATTraversalNotify End\n")));
    return 0;
}

int NSClientProxy::NATTraversalReq(const DevID_t& targetId, const IData& data)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) NATTraversalReqq Start\n")));

    //BCSClientHandler *handler = (BCSClientHandler *)this->GenConn();
    ClientHandler *handler = this->GenConn();//NULL;

    MsgHandler *msgHandler = MsgHandlerFactory::instance()->GetMsgHandler(NSC_PASSTHROUGH);
    if(msgHandler == NULL)
    {
        return -1;
    }

    //PassThrough argv
    PassThroughArgv argvPT;
    argvPT.s_DataType = REQ;
    argvPT.s_Cmd = NSC_PT_NATTRAVERSALREQ;
    argvPT.s_DevID = targetId;
    argvPT.s_SubArgv = (void *)&data;
    string buff = msgHandler->CreateMsg((void *)&argvPT);

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) Msg: %s\n"), buff.c_str()+16));
    handler->SendMsg(buff.c_str(), buff.size());

    return 0;
}

int NSClientProxy::NATTraversalReqAck(const DevID_t& targetId, const void *data)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) NATTraversalReqAck Start\n")));

    //BCSClientHandler *handler = (BCSClientHandler *)this->GenConn();
    ClientHandler *handler = this->GenConn();//NULL;

    MsgHandler *msgHandler = MsgHandlerFactory::instance()->GetMsgHandler(NSC_PASSTHROUGH);
    if(msgHandler == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get Msghandler failed!\n")), -1);
    }

    PassThroughArgv argvPT;
    argvPT.s_DataType = RESP;
    argvPT.s_Cmd = NSC_PT_NATTRAVERSALREQ;
    argvPT.s_DevID = targetId;

    //MH_PT_NATTraversalReq::TraversalReqStruct subArgv;
    argvPT.s_SubArgv = (void *)data;
    string buff = msgHandler->CreateMsg((void *)&argvPT);

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) Msg: %s\n"), buff.c_str()+16));
    handler->SendMsg(buff.c_str(), buff.size());
}

int NSClientProxy::NATTraversalResp(const DevID_t& targetId, const IData& data)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) NATTraversalResp Start\n")));

    //BCSClientHandler *handler = (BCSClientHandler *)this->GenConn();
    ClientHandler *handler = this->GenConn();//NULL;

    MsgHandler *msgHandler = MsgHandlerFactory::instance()->GetMsgHandler(NSC_PASSTHROUGH);
    if(msgHandler == NULL)
    {
        return -1;
    }

    //PassThrough argv
    PassThroughArgv argvPT;
    argvPT.s_DataType = REQ;
    argvPT.s_Cmd = NSC_PT_NATTRAVERSALRESP;
    argvPT.s_DevID = targetId;
    argvPT.s_SubArgv = (void *)&data;
    string buff = msgHandler->CreateMsg((void *)&argvPT);

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) Msg: %s\n"), buff.c_str()+16));
    handler->SendMsg(buff.c_str(), buff.size());

    return 0;
}

int NSClientProxy::NATTraversalRespAck(const DevID_t& targetId, const void *data)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) NATTraversalRespAck Start\n")));

    //BCSClientHandler *handler = (BCSClientHandler *)this->GenConn();
    ClientHandler *handler = this->GenConn();//NULL;

    MsgHandler *msgHandler = MsgHandlerFactory::instance()->GetMsgHandler(NSC_PASSTHROUGH);
    if(msgHandler == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get Msghandler failed!\n")), -1);
    }

    PassThroughArgv argvPT;
    argvPT.s_DataType = RESP;
    argvPT.s_Cmd = NSC_PT_NATTRAVERSALRESP;
    argvPT.s_DevID = targetId;

    //MH_PT_NATTraversalReq::TraversalReqStruct subArgv;
    argvPT.s_SubArgv = (void *)data;
    string buff = msgHandler->CreateMsg((void *)&argvPT);

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) Msg: %s\n"), buff.c_str()+16));
    handler->SendMsg(buff.c_str(), buff.size());
}

int NSClientProxy::NATTraversalResuAck(const void *argv)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) NATTraversalResuAck Start\n")));

    PassThroughArgv *oldArgv = (PassThroughArgv *)argv;

    //BCSClientHandler *handler = (BCSClientHandler *)this->GenConn();
    ClientHandler *handler = this->GenConn();//NULL;

    MsgHandler *msgHandler = MsgHandlerFactory::instance()->GetMsgHandler(NSC_PASSTHROUGH);
    if(msgHandler == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get Msghandler failed!\n")), -1);
    }

    PassThroughArgv argvPT;
    argvPT.s_DataType = RESP;
    argvPT.s_Cmd = NSC_PT_NATTRAVERSALRESU;
    argvPT.s_DevID = oldArgv->s_DevID;

    //MH_PT_NATTraversalReq::TraversalReqStruct subArgv;
    argvPT.s_SubArgv = oldArgv->s_SubArgv;
    string buff = msgHandler->CreateMsg((void *)&argvPT);

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) Msg: %s\n"), buff.c_str()+16));
    handler->SendMsg(buff.c_str(), buff.size());
}

int NSClientProxy::NATTraversalStop(const DevID_t dev, const string& sid)
{
    //BCSClientHandler *handler = (BCSClientHandler *)this->GenConn();
    ClientHandler *handler = this->GenConn();//NULL;

    MsgHandler *msgHandler = MsgHandlerFactory::instance()->GetMsgHandler(NSC_PASSTHROUGH);
    if(msgHandler == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get Msghandler failed!\n")), -1);
    }

    PassThroughArgv argvPT;
    argvPT.s_DataType = REQ;
    argvPT.s_SidStr = sid; 
    argvPT.s_Cmd = NSC_PT_NATTRAVERSALSTOP;
    argvPT.s_DevID = dev;

    string buff = msgHandler->CreateMsg((void *)&argvPT);

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) Msg: %s\n"), buff.c_str()+16));
    handler->SendMsg(buff.c_str(), buff.size());
}

void NSClientProxy::Clean()
{
    DevObj *tmpDev = NULL;
    DevsVector_t::iterator it = m_Devices.begin();
    while(it != m_Devices.end())
    {
        tmpDev = DevRepo::instance()->GetDevObj(*it);
        if(tmpDev != NULL)
        {
            tmpDev->NSProxy(NULL);
        }
        it = m_Devices.erase(it);
    }
}
