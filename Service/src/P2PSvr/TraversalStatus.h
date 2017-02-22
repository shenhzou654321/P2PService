#ifndef __TRAVERSALSTATUS_H__
#define __TRAVERSALSTATUS_H__
#include "ProtocolLib/JBody.h"
#include "ProtocolLib/Head.h"
#include "Units.h"
#include "PSUnits.h"

class TraversalSessionObj;

class TraversalStatus
{
    public:
        typedef struct BaseData_t
        {
            IData       s_Data;
        } BaseData;

        TraversalStatus(TraversalSessionObj *sess) : m_Next(NULL), m_Sess(sess) 
        {
            m_MsgTimer = m_MsgTimer.now();
            m_FirstSendMsgFlag = true;
        }

        TS_Value StatusValue()
        {
            return m_TS_Value;
        }

        TraversalStatus *Next()
        {
            return m_Next;
        }

        virtual void HandleMsg(void *); 
        virtual void Work() {}
    protected:
        TraversalStatus         *m_Next;
        TraversalSessionObj     *m_Sess;
        TS_Value                m_TS_Value;
        ACE_Time_Value          m_MsgTimer;    //消息发送时间
        bool                    m_FirstSendMsgFlag;
};

class TS_TraversalInit: public TraversalStatus
{
    public:
        TS_TraversalInit(TraversalSessionObj *sess) : TraversalStatus(sess)
    {
        m_TS_Value = TS_INIT;
    }
    /**
     * @brief Work 工作线程：判断相关设备是否满足穿透条件
     */
        virtual void Work();
};

class TS_Prepare : public TraversalStatus
{
    public:
        typedef struct TS_PrepareStruct_t 
        {
            int         result;
            SessionId   sid;

            TS_PrepareStruct_t(int res, const char *id) :
                result(res), sid(id)
            {}
        } TS_PrepareStruct;
    TS_Prepare(TraversalSessionObj *sess) : TraversalStatus(sess) 
    {
        m_TS_Value = TS_PREPARE;
    }
    /**
     * @brief Work 发送NATTraversalNotify消息
     */
    virtual void Work();
    virtual void HandleMsg(void *);
};

class TS_TraversalReq : public TraversalStatus
{
    public:
        TS_TraversalReq(TraversalSessionObj *sess) : TraversalStatus(sess), m_ReqFlag(false)
        {
            m_TS_Value = TS_REQ;
        }
        void HandleMsg(void *msg);
        virtual void Work();
    private:
        bool            m_ReqFlag;          //处理完成TraversalReq消息标识
        IData           m_Data;                  //需要转发的消息
};

class TS_TraversalResp : public TraversalStatus
{
    public:
        TS_TraversalResp(TraversalSessionObj *sess) : TraversalStatus(sess), m_ReqFlag(false)
        {
            m_TS_Value = TS_RESP;
        }
        void HandleMsg(void *msg);
        virtual void Work();
    private:
        bool            m_ReqFlag;          //处理完成TraversalResp消息标识
        IData           m_Data;                  //需要转发的消息
};

class TS_TraversalWait : public TraversalStatus
{
    public:
        TS_TraversalWait(TraversalSessionObj *sess) : TraversalStatus(sess)//, m_ReqFlag(false)
        {
            m_TS_Value = TS_RESP;
        }

        void HandleMsg(void *msg);
        virtual void Work(){};
};

class TS_TraversalHB : public TraversalStatus
{
    public:
        TS_TraversalHB(TraversalSessionObj *sess) : TraversalStatus(sess)
        {
            m_TS_Value = TS_HB;
        }
        void HandleMsg(void *msg);
};

class TS_TraversalDie: public TraversalStatus
{
    public:
        TS_TraversalDie(TraversalSessionObj *sess) : TraversalStatus(sess)
        {
            m_TS_Value = TS_DIE;
        }
};

class TS_TraversalEND : public TraversalStatus
{
    public:
        TS_TraversalEND(TraversalSessionObj *sess) : TraversalStatus(sess)
        {
            m_TS_Value = TS_END;
        }

        void HandleMsg(void *msg);
        virtual void Work();
//    private:
//        bool            m_ReqFlag;          //处理完成TraversalResp消息标识
};

#endif //__TRAVERSALSTATUS_H__
