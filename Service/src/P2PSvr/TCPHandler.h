/**
 * @file TCPHandler.h
 * @brief 服务器处理类（包括处理句柄及触发机制)
 * @author pengcheng<pengcheng@comtom.cn>
 * @version 1.0.0
 * @date 2015-12-10
 */
#ifndef __TCPHANDLER_H__        
#define __TCPHANDLER_H__

#include <string>
#include <iostream>
#include "ace/Acceptor.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Svc_Handler.h"
#include "ace/SOCK_Stream.h"
#include "ace/Reactor_Notification_Strategy.h"
#include "ace/Dev_Poll_Reactor.h"
#include "PSThread.h"
#include "PSUnits.h"

/**
 * @brief 服务器处理句柄
 */
class ServerHandler : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
{
    typedef ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH> super;

    public:
        virtual int open(void * = 0);
        virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE);
        virtual int close(u_long);
        int HandleMessage(const unsigned char *buff, int size);

    private:
        ACE_TCHAR peer_name[MAXHOSTNAMELEN];
};

/**
 * @brief 服务器处理类(接收消息，分析、处理<调用其他消息处理器处理>
 */
class TCPHandler : public PSThread
{
    public:
        TCPHandler() : PSThread("TCPHandler"), m_Reactor(NULL), m_PreFlag(false) {}
        ~TCPHandler()
        {
            delete m_Reactor;
        }

        static TCPHandler *instance();
        void PreProccess(string ipAddr);

        /**
         * @brief Init 初始化：开启监听端口
         *
         * @return 
         */
        int Init();
        virtual int svc(void);
    private:
        ACE_Acceptor<ServerHandler, ACE_SOCK_ACCEPTOR> m_TCPServer;
        ACE_Reactor             *m_Reactor;
        ACE_INET_Addr           m_IPAddr;
        bool                    m_PreFlag;
};


#endif //__TCPHANDLER_H__
