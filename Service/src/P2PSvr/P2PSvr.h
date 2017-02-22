#ifndef __P2PSVR_H__
#define __P2PSVR_H__

#include "TCPHandler.h"
#include "ClientProxy.h"
#include "Device.h"
#include "MsgHandler.h"

class P2PSvr
{
    public:
        P2PSvr();
        ~P2PSvr();

        void Init(string &BCSAddr);
        void Start();
        void Run();
        void Fini();
        void Stop();

        void STUNIp(string ip)
        {
            m_STUNIp = ip;
        }
        string STUNIp()
        {
            return m_STUNIp;
        }
        void STUNPort(int port)
        {
            m_STUNPort = port;
        }
        int STUNPort()
        {
            return m_STUNPort;
        }

        //ACE_Time_Value *Timer() {return m_Timer};

    private:
        TCPHandler              *m_TCPHandler;
        BCSClientProxy          *m_BCSClientProxy;
        MsgHandlerFactory       *m_MsgHandlerFactory;
        NSClientRepo            *m_NSClientRepo;
        DevRepo                 *m_DevRepo;
        TraversalSessionRepo    *m_SessionRepo;
        MsgSender               *m_HBSender;

        ACE_INET_Addr   m_IPAddr;
        bool            m_Running;

        string          m_STUNIp;
        int             m_STUNPort;

        //time_t          m_Timer;
        //ACE_Time_Value m_Timer;
};

extern P2PSvr theApp;

#endif //__P2PSVR_H__
