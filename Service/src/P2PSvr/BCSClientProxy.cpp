#include "ProtocolLib/JBody.h"
#include "ProtocolLib/Head.h"
#include "PSUnits.h"
#include "ClientProxy.h"
#include "MsgHandler.h"

static BCSClientProxy _ins_BCSClientProxy;

BCSClientProxy *BCSClientProxy::instance()
{
    return (&_ins_BCSClientProxy);
}

ClientHandler *BCSClientProxy::GenConn()
{
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) BCSClientProxy GenConn\n")));

    //BCSClientHandler *handler = new BCSClientHandler();//NULL;
    ClientHandler *handler = new ClientHandler();
    if(m_Client.connect(handler, m_IPAddr) == -1)
    {
        ACE_ERROR((LM_ERROR, ACE_TEXT("[%D](%P|%t) Connection failed\n")));
        return NULL;
    }
    return handler;
}

int BCSClientProxy::svc(void)
{
    ACE_Reactor *reactor = ACE_Reactor::instance();
    reactor->owner(ACE_Thread::self());
    reactor->run_reactor_event_loop();
}

int BCSClientProxy::HeartBeat()
{
    ClientHandler *handler = this->GenConn();
    MsgHandler *msgHandler = MsgHandlerFactory::instance()->GetMsgHandler(BCSC_HEARTBEAT);
    if(msgHandler == NULL)
    {
        return -1;
    }

    string buff = msgHandler->CreateMsg();
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Msg: %s\n"), buff.c_str()));
    handler->SendMsg(buff.c_str(), buff.size());
}

/**
 * @brief GetLocalConfigureInfo 获取配置请求（到BCS）
 *
 * @return 状态（0：正常，小于0：异常）
 */
int BCSClientProxy::GetLocalConfigureInfo()
{
    ClientHandler *handler = (ClientHandler *)this->GenConn();
    MsgHandler *msgHandler = MsgHandlerFactory::instance()->GetMsgHandler(BCSC_GETCONFIG);
    if(msgHandler == NULL)
    {
        return -1;
    }

    MH_GetConfig::GetConfigArgv argv;
    argv.s_ServerType = AbbreviationNameForPS;
    argv.s_QueryData = "self";
    string buff = msgHandler->CreateMsg((void *)&argv);

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Msg: %s\n"), buff.c_str()));
    handler->SendMsg(buff.c_str(), buff.size());

    return 0;
}

int BCSClientProxy::GetNSSvrInfo()
{
    this->GetSvrInfo(AbbreviationNameForNS);
}

int BCSClientProxy::GetSTUNSvrInfo()
{
    this->GetSvrInfo(AbbreviationNameForSTUN);
}

int BCSClientProxy::GetSvrInfo(string svrType)
{
    ClientHandler *handler = (ClientHandler *)this->GenConn();
    MsgHandler *msgHandler = MsgHandlerFactory::instance()->GetMsgHandler(BCSC_GETCONFIG);
    if(msgHandler == NULL)
    {
        return -1;
    }

    MH_GetConfig::GetConfigArgv argv;
    argv.s_ServerType = svrType;
    //argv.s_QueryData = "self";
    string buff = msgHandler->CreateMsg((void *)&argv);

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Msg: %s\n"), buff.c_str()));
    handler->SendMsg(buff.c_str(), buff.size());

    return 0;
}

int BCSClientProxy::GetNodesTopologyInfo()
{
    ClientHandler *handler = (ClientHandler *)this->GenConn();
    MsgHandler *msgHandler = MsgHandlerFactory::instance()->GetMsgHandler(BCSC_GETNODESTOPOLOGYINFO);
    if(msgHandler == NULL)
    {
        return -1;
    }
    string buff = msgHandler->CreateMsg();

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) Msg: %s\n"), buff.c_str()));
    handler->SendMsg(buff.c_str(), buff.size());

    return 0;
}

int BCSClientProxy::StartUpNotify()
{
    //BCSClientHandler *handler = (BCSClientHandler *)this->GenConn();
    ClientHandler *handler = (ClientHandler *)this->GenConn();
    MsgHandler *msgHandler = MsgHandlerFactory::instance()->GetMsgHandler(BCSC_NOTIFY);
    if(msgHandler == NULL)
    {
        return -1;
    }

    string buff = msgHandler->CreateMsg(NULL);

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Msg: %s\n"), buff.c_str()));
    handler->SendMsg(buff.c_str(), buff.size());

    return 0;
}

int BCSClientProxy::DataLoad()
{
    this->GetNSSvrInfo();
    this->GetSTUNSvrInfo();
    this->GetNodesTopologyInfo();
    this->StartUpNotify();
}
