#ifndef __CLIENTHANDLER_H__
#define __CLIENTHANDLER_H__

#include "ace/Svc_Handler.h"
#include "ace/SOCK_Stream.h"
#include "ace/Reactor_Notification_Strategy.h"
#include "PSThread.h"
//#include "MsgHandler.h"

class ClientHandler : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
{
    typedef ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH> super;
    public:
        ClientHandler() {}
        virtual int open(void * = 0);

        virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE);
        virtual int handle_timeout(const ACE_Time_Value &currentTime, const void *act = 0);
        int SendMsg(const char *msg, size_t len);
        virtual int HandleMessage(const unsigned char *buffer, int size) ;
};

#if 1 
class MsgSender:public PSThread//, public ACE_Event_Handler
{
    public:
        MsgSender(int interTime) : PSThread("HBSender"), m_InterTime(interTime) {};

        virtual int svc(void);
        virtual int handle_timeout(const ACE_Time_Value &currentTime, const void *act = 0);
    private:
        int             m_InterTime;
};
#else
#if 0
class ScheduleTimer : public ACE_Event_Handler
{
    public:
        virtual int handle_timeout(const ACE_Time_Value &currentTime, const void *act = 0);
};

class MsgSender : public PSThread 
{
    public:
        MsgSender(int interTime) : PSThread("HBSender"), m_InterTime(interTime) {};

        virtual int svc(void);
    private:
        int             m_InterTime;
};
#endif

#endif


#endif
