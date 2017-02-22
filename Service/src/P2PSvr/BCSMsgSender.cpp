#include "ProtocolLib/JBody.h"
#include "ProtocolLib/Head.h"
#include "PSUnits.h"
#include "ClientHandler.h"
#include "MsgHandler.h"

int MsgSender::svc()
{
    ACE_Time_Value iter_delay(m_InterTime);

#if 1
    ACE_Reactor *reactor = ACE_Reactor::instance();
    reactor->owner(ACE_Thread::self());
    reactor->schedule_timer(this, 0, ACE_Time_Value::zero, iter_delay);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) MsgSender svc \n")));
    reactor->run_reactor_event_loop();
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) MsgSender svc end\n")));
#else
    this->reactor(ACE_Reactor::instance());
    this->reactor()->schedule_timer(this, 0, ACE_Time_Value::zero, iter_delay);

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|T5) MsgSender svc \n")));
    ACE_Reactor::instance()->run_reactor_event_loop();
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|T5) MsgSender svc end\n")));
#endif
}

int MsgSender::handle_timeout(const ACE_Time_Value &, const void *)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) MsgSender handleTimeOut\n")));
    BCSClientProxy::instance()->HeartBeat();
    return 0;
}
