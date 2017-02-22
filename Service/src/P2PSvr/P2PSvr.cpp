#include "P2PSvr.h"
#include "ClientProxy.h"

P2PSvr theApp;

P2PSvr::P2PSvr() : m_Running(false)
{
    m_TCPHandler = TCPHandler::instance();
    m_BCSClientProxy = BCSClientProxy::instance();
    m_MsgHandlerFactory = MsgHandlerFactory::instance();
    //m_Timer = new ACE_Time_Value(ACE_OS::gettimeofday());
    m_NSClientRepo = NSClientRepo::instance();
    m_DevRepo = DevRepo::instance();
    m_SessionRepo = TraversalSessionRepo::instance();
    m_HBSender = new MsgSender(30);
}

P2PSvr::~P2PSvr()
{
}

void P2PSvr::Init(string &BCSAddr)
{
    //初始化BCSProxy，并启动线程
    m_BCSClientProxy->Init(BCSAddr);
    m_BCSClientProxy->Start();
    m_MsgHandlerFactory->Init();
    //获取PS AddrInfo
    m_BCSClientProxy->GetLocalConfigureInfo();
    //初始化TCPHandler
    if((m_TCPHandler->Init()) == -1)
    {
        exit(-1);
    }

    m_Running = true;
}

void P2PSvr::Start()
{
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) P2PSvr Start...\n")));
    m_TCPHandler->Start();
    m_DevRepo->Start();
    m_SessionRepo->Start();
    m_HBSender->Start();
}

void P2PSvr::Run()
{
    //ACE_Time_Value timer;
    m_BCSClientProxy->DataLoad();
    while(m_Running)
    {
        //ACE_DEBUG((LM_DEBUG, ACE_TEXT("P2PSvr Running...\n")));
        ACE_OS::sleep(1);
#if 0
        timer = timer.now();
        if(BCS_HB_INTERVAL  <= (timer - m_Timer).sec())
        {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) HeartBeat...\n")));
            BCSClientProxy::instance()->HeartBeat();
            m_Timer = m_Timer.now();
        }
#endif
    }
}

void P2PSvr::Fini()
{
    m_MsgHandlerFactory->Fini();
}

void P2PSvr::Stop()
{
    m_Running = false;
}
