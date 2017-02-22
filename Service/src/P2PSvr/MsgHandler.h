#ifndef __MSGHANDLER_H__
#define __MSGHANDLER_H__

#include <map>
#include <string>
#include "ace/Svc_Handler.h"
#include "ace/SOCK_Stream.h"
using std::map;
using std::string;
using std::stringstream;

#include "ProtocolLib/JBody.h"
#include "ProtocolLib/Head.h"

#include "Units.h"
#include "Device.h"

class MsgHandler;

typedef enum {
    REQ = 0,    //构建请求消息
    RESP       //构建回应消息
} DataType;
typedef struct PassThroughArgv_t
{
    DataType    s_DataType;     //消息类型
    string      s_Cmd;
    DevID_t     s_DevID; //消息目标设备ID
    string      s_SidStr;

    void        *s_SubArgv;
} PassThroughArgv;

/**
 * @brief 消息处理工厂类（Message Handler）
 */
class MsgHandlerFactory
{
    public:
        static MsgHandlerFactory *instance();
        MsgHandler *GetMsgHandler(const JBody &body);
        MsgHandler *GetMsgHandler(const char *cmd);
        MsgHandler *GetMsgHandler(const IData& data);
        void Init();
        void Fini();
    private:
        map<string, MsgHandler *> m_MsgHandlers;
};

/**
 * @brief 消息处理类（基类）
 */
class MsgHandler
{
    public:
        //MsgHandler(const JBody &body);
        virtual int HandleMsg(void *jbodyArgv, ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler = NULL){};
        string ResponseBaseMsg(JBody *body, 
                bool isAck = false, 
                int retCode = 200, 
                const char *retMes = "OK", 
                IData *data = NULL);
        virtual string CreateMsg(void *argv = NULL) {};
    protected:
        //JBody           m_Body;
        //uint16_t        m_Len;
};

/**
 * @brief 消息处理类（Message Handle）：获取配置信息
 */
class MH_GetConfig : public MsgHandler
{
    public:
        virtual int HandleMsg(void *jbodyArgv, ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler = NULL);
        virtual string CreateMsg(void *argv = NULL);

        typedef struct GetConfigArgv_t
        {
            string s_ServerType;
            string s_QueryData;

            GetConfigArgv_t() : 
                s_ServerType(""), s_QueryData("") 
            {}
        } GetConfigArgv;
};

/**
 * @brief 消息处理类（Message Handle）：心跳
 */
class MH_HeartBeat : public MsgHandler
{
    public:
        virtual int HandleMsg(void *jbodyArgv, ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler = NULL);
        virtual string CreateMsg(void *argv = NULL);
};

/**
 * @brief 消息处理类（Message Handle）：获取节点拓扑信息
 */
class MH_GetNodesTopologyInfo : public MsgHandler
{
    public:
        virtual int HandleMsg(void *jbodyArgv, ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler = NULL);
        virtual string CreateMsg(void *argv = NULL);
};

/**
 * @brief 消息处理类（Message Handle）：更新节点信息
 */
class MH_UpdateNodesInfo: public MsgHandler
{
    public:
        virtual int HandleMsg(void *jbodyArgv, ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler = NULL);
};

/**
 * @brief 消息处理类（Message Handler）：获取会话中转节点
 */
class MH_GetNodesForPlay : public MsgHandler
{
    public:
        virtual int HandleMsg(void *jbodyArgv, ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler = NULL);
        virtual string CreateMsg(void *argv = NULL);
};

/**
 * @brief 消息处理类（Message Handler）：启动通知类
 */
class MH_Notify : public MsgHandler
{
    public:
        virtual int HandleMsg(void *jbodyArgv, ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler = NULL);
        virtual string CreateMsg(void *argv = NULL);
};

/**
 * @brief 消息处理类（Message Handle）：设备状态同步
 */
class MH_DevStatusSync: public MsgHandler
{
    public:
        virtual int HandleMsg(void *jbodyArgv, ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler = NULL);
};

/**
 * @brief 消息处理类（Message Handle）：设备状态汇报
 */
class MH_DevStatusReport: public MsgHandler
{
    public:
        virtual int HandleMsg(void *jbodyArgv, ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler = NULL);
};

/**
 * @brief 消息处理类（Message Handle）：中转消息
 */
class MH_PassThrough : public MsgHandler
{
    public:
        virtual int HandleMsg(void *jbodyArgv, ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler = NULL);
        virtual string CreateMsg(void *argv = NULL);

#if 0
        typedef struct PT_SubData_Argv_t
        {
            DevID_t     s_Source;
            IData       *s_Data;
        } PT_SubData_Argv;
#endif

};

/**
 * @brief 消息处理类（Message Handle）：中转消息之NAT穿透通知
 */
class MH_PT_NATTraversalNotify: public MsgHandler
{
    public:
        virtual int HandleMsg(void *jbodyArgv, ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler = NULL);
        virtual string CreateMsg(void *argv = NULL);

        typedef struct NATTraversalNotify_t
        {
            SessionId   s_SID;
            DevID_t     s_DevID;     
            string      s_STUNIP;
            int         s_STUNPort;

            NATTraversalNotify_t(SessionId sid, DevID_t devId, string ip, int port) :
                s_SID(sid), s_DevID(devId), s_STUNIP(ip), s_STUNPort(port)
            {}
        } NATTraversalNotify;
};

/**
 * @brief 消息处理类（Message Handle）：中转消息之NAT穿透请求
 */
class MH_PT_NATTraversalReq: public MsgHandler
{
    public:
        virtual int HandleMsg(void *jbodyArgv, ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler = NULL);
        virtual string CreateMsg(void *argv = NULL);
};

/**
 * @brief 消息处理类（Message Handle）：中转消息之NAT穿透回应
 */
class MH_PT_NATTraversalResp: public MsgHandler
{
    public:
        virtual int HandleMsg(void *jbodyArgv, ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler = NULL);
        virtual string CreateMsg(void *argv = NULL);
};

/**
 * @brief 消息处理类（Message Handle）：中转消息之NAT穿透结果汇报
 */
class MH_PT_NATTraversalResu: public MsgHandler
{
    public:
        virtual int HandleMsg(void *jbodyArgv, ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler = NULL);
        virtual string CreateMsg(void *argv = NULL);
};

/**
 * @brief 消息处理类（Message Handle）：中转消息之NAT穿透停止
 */
class MH_PT_NATTraversalStop: public MsgHandler
{
    public:
        virtual int HandleMsg(void *jbodyArgv, ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler = NULL);
        virtual string CreateMsg(void *argv = NULL);
};

/**
 * @brief 消息处理类（Message Handle）：中转消息之重新NAT穿透
 */
class MH_PT_NodesTraversalRetry: public MsgHandler
{
    public:
        virtual int HandleMsg(void *jbodyArgv, ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>  *handler = NULL);
};
#endif
