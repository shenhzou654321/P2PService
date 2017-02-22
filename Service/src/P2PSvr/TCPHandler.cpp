#include "ace/Reactor.h"
#include "TCPHandler.h"

static TCPHandler _ins_TCPHandler;

TCPHandler *TCPHandler::instance()
{
    return (&_ins_TCPHandler);
}

void TCPHandler::PreProccess(string ipAddr)
{
    m_IPAddr.set(ipAddr.c_str());
    m_PreFlag = true;
}

int TCPHandler::Init()
{
    const int loopNum = 5;
    for(int i=0; i<10; i++)
    {
        if(m_PreFlag)
            break;
        ACE_OS::sleep(ACE_Time_Value(0, 500 * 1000));    //sleep 50 ms; 
    }
    if(!m_PreFlag)
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("[%D](%P|%t) TCPHandler init failed! Get P2PSvr's ip address failed!\n")), -1);

    ACE_Dev_Poll_Reactor *epollReactor = NULL;
    try {
        epollReactor = new ACE_Dev_Poll_Reactor(3000);
    }
    catch(...)
    {
        return -1;
    }

    try {
        m_Reactor = new ACE_Reactor(epollReactor);
    }
    catch(...)
    {
        return -1;
    }
    int flags = 0;
    ACE_SET_BITS(flags, ACE_NONBLOCK);

    ACE_TCHAR addr[MAXHOSTNAMELEN];
    m_IPAddr.addr_to_string(addr, MAXHOSTNAMELEN);

    if(m_TCPServer.open(m_IPAddr, m_Reactor, flags) == -1)
    {
        ACE_ERROR((LM_ERROR, ACE_TEXT("[%D](%P|%t) TCPHandler listen %s failed!\n"), addr));
        return -1;
    }
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TCPHandler listen %s succ!\n"), addr));
    return 0;
}

int TCPHandler::svc(void) 
{
    m_Reactor->owner(ACE_Thread::self());
    m_Reactor->run_reactor_event_loop();
}

