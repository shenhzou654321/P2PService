#ifndef __CLIENTPROXY_H__
#define __CLIENTPROXY_H__

#include <string>
#include <map>
#include "ace/Connector.h"
#include "ace/SOCK_Connector.h"
#include "ProtocolLib/JBody.h"
#include "ProtocolLib/Head.h"
#include "PSThread.h"
#include "ClientHandler.h"
#include "PSUnits.h"
#include "Units.h"

using std::string;
using namespace std;

class ClientProxy : public PSThread//, public RefCounter
{
    public:
        ClientProxy() : PSThread("ClientProxy") 
        {
            ACE_Reactor *reactor = new ACE_Reactor();
            ACE_Task_Base::reactor(reactor);
        }
        ClientProxy(string svrName) : PSThread(svrName.c_str()) 
        {
            ACE_Reactor *reactor = new ACE_Reactor();
            ACE_Task_Base::reactor(reactor);
        }
        ~ClientProxy()
        {
            delete ACE_Reactor::instance();
        }

        bool IsThisProxy(ACE_INET_Addr &addr)
        {
            return m_IPAddr == addr;
        }

        virtual void Init(string ipAddr)
        {
            m_IPAddr.set(ACE_INET_Addr(ipAddr.c_str()));
        }

        virtual void Init(ACE_INET_Addr &ipAddr)
        {
            m_IPAddr.set(ipAddr);
        }

        virtual void Start()
        {
            PSThread::Start();
        }

        virtual int svc(void)
        {
            ACE_Reactor *reactor = ACE_Reactor::instance();
            reactor->owner(ACE_Thread::self());
            reactor->run_reactor_event_loop();
        }
    protected:
        ACE_INET_Addr           m_IPAddr;
};

class BCSClientProxy : public ClientProxy 
{
    public:
        BCSClientProxy() : ClientProxy("BCSClientProxy") {}
        static BCSClientProxy *instance();
        virtual int svc(void);

        /**
         * @brief GetLocalConfigureInfo 获取配置信息（PS IP及端口等）
         *
         * @return 成功返回0
         */
        int GetLocalConfigureInfo();

        /**
         * @brief GetNodesTopologyInfo 获取系统节点拓扑结构
         *
         * @return 
         */
        int GetNodesTopologyInfo();
        /**
         * @brief GetNSSvrInfo 获取NS Svr信息（向BCS） 
         *
         * @return 结果（-1：失败，0：成功）
         */
        int GetNSSvrInfo();

        /**
         * @brief GetSTUNSvrInfo 获取STUN服务器信息
         *
         * @return 
         */
        int GetSTUNSvrInfo();

        /**
         * @brief GetSvrInfo 获取服务器信息
         *
         * @param svrType 服务器类型
         *
         * @return 
         */
        int GetSvrInfo(string svrType);

        /**
         * @brief StartUpNotify 通知BCS：PS启动
         *
         * @return 
         */
        int StartUpNotify();

        int DataLoad();
        int HeartBeat();
    protected:
        virtual ClientHandler *GenConn();
    protected:
        //ACE_Connector<BCSClientHandler, ACE_SOCK_Connector> m_Client;
        ACE_Connector<ClientHandler, ACE_SOCK_Connector> m_Client;

};

class NSClientProxy : public ClientProxy
{
    public:
        NSClientProxy() : ClientProxy("NSClientProxy") 
        {
            m_RegisterTime = m_RegisterTime.now();
        }
        ~NSClientProxy()
        {
            this->Clean();
        }
        void Clean();
        void UpdateTimer() 
        {
            m_RegisterTime = m_RegisterTime.now();
        }
        ACE_Time_Value Timer()
        {
            return m_RegisterTime;
        }

        //int PassThroughMsg(UUID_t tid, DevID_t devID, string ip, int port);
       // int NATTraversalNotify(DevID_t targetId, string ip, int port, DevID_t traversalTargetId, SessionId &sid);
        int NATTraversalNotify(const DevID_t& targetId, const string& ip, const int port, DevID_t traversalTargetId,const  SessionId& sid);
        int NATTraversalReq(const DevID_t& targetId, const IData& data);
        int NATTraversalReqAck(const DevID_t& targetId, const void  *data);
        int NATTraversalResp(const DevID_t& targetId, const IData& data);
        int NATTraversalRespAck(const DevID_t& targetId, const void *data);
        int NATTraversalResuAck(const void *argv);
        int NATTraversalStop(const DevID_t dev, const string& sid);

    protected:
        virtual ClientHandler *GenConn();
    protected:
        ACE_Connector<ClientHandler, ACE_SOCK_Connector> m_Client;
    private:
        ACE_Time_Value          m_RegisterTime;
        DevsVector_t            m_Devices; 
};

class NSClientRepo// : public PSThread
{
    public:
        NSClientRepo(){}; //: PSThread("NSClientRepo") {};
        static NSClientRepo *instance();
        //virtual int svc(void);
         /**
         * @brief UpdateNSInfo 更新NS信息
         *     NS向PS发送消息时，触发 
         *
         * @param devId
         * @param NSIPInfo NS服务器IP信息（NAT IP）
         *
         * @return 
         */
        NSClientProxy *UpdateNSInfo(DevID_t devId, ACE_INET_Addr &ipAddr);
        
        /**
         * @brief UpdateNSProxys 更新NS服务器对象信息（向BCS获取后进行更新）
         *
         * @param list NS地址列表
         *
         * @return 异常代码（正常为0）
         */
        int UpdateNSProxys(std::vector<ACE_INET_Addr> list);
        //int NewNSProxys(std::vector<ACE_INET_Addr> list);

        //typedef std::map<DevID_t, NSClientProxy *> DevOwner_t;
        typedef std::map<uint32_t, NSClientProxy *> IPNSClientRepo_t;
    private:
//        void CleanDevOwners(NSClientProxy *proxy);
    private:
        //DevOwner_t              m_DevOwner;     //设备与NS对应列表
        //ACE_RW_Thread_Mutex     m_DO_Mutex;     //m_DevOwner锁
        ACE_RW_Thread_Mutex     m_Mutex;        
        IPNSClientRepo_t              *m_NSClientRepo;       //NS管理器
};

#endif
