#include "TraversalStatus.h"
#include "Device.h"
#include "P2PSvr.h"

extern P2PSvr theApp;

void TraversalStatus::HandleMsg(void *argv)
{
    IData data = (*(IData *)argv);

    if(!data.isMember(KW_SUBCMD))
    {
        return;
    }

    //接收到TraversalStop回应，进入END状态
    if(BCSC_RESPONSE == data[KW_SUBCMD].asString()
            && data.isMember(KW_SUBACK)
            && NSC_PT_NATTRAVERSALSTOP == data[KW_SUBACK].asString()
            && TS_END != m_Sess->StatusValue()
            && TS_END != m_Next->StatusValue())
    {
        //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TS_TraversalStop  into TS_End\n")));
        m_Next = new TS_TraversalDie(m_Sess);
    }

    if(NSC_PT_NATTRAVERSALRESU == data[KW_SUBCMD].asString()
            && data.isMember(KW_RESULT))
    {
        int ret = data[KW_RESULT].asInt();
        switch(ret)
        {
            case RES_CODE_OK:
                {
                    m_Next = new TS_TraversalHB(m_Sess);
                    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TraversalSess Status into next status[HB]!\n")));
                    m_Sess->IntoSessionTraversaled();
                    break;
                }
            case RES_CODE_TS_TIMEOUT:
                ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TraversalSess Status into next status[END], Result[Traversal Timeout]!\n")));
            case RES_CODE_TS_FAILED:
                {
                    m_Next = new TS_TraversalEND(m_Sess);
                    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TraversalSess Status into next status[END], Result[Traversal Failed]!\n")));
                }
                break;
        }
    }
}

void TS_TraversalInit::Work()
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) TS_TraversalInit Work\n")));

    DevObj *source = DevRepo::instance()->GetDevObj(m_Sess->SourceID());
    DevObj *target = DevRepo::instance()->GetDevObj(m_Sess->TargetID());
    if(source == NULL || target == NULL || m_Sess->BreakOutFlag())
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Device is null or BreakOuted!\n")));
        m_Next = new TS_TraversalEND(m_Sess);
        return;
    }

    ACE_RW_Thread_Mutex *tmpSourceMutex = source->DevStatusMutex();
    ACE_RW_Thread_Mutex *tmpTargetMutex = target->DevStatusMutex();

    ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(*tmpSourceMutex);
    ACE_Write_Guard<ACE_RW_Thread_Mutex> guard2(*tmpTargetMutex);
    if(source->IsFree() && target->IsFree())
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TS_TraversalInit Work DevFree\n")));
        m_Next = new TS_Prepare(m_Sess);
        m_Sess->IntoSessionTraversaling();
    }
}

void TS_Prepare::Work()
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) TS_Prepare[%s] \n"), m_Sess->TraversalSessionID().tostr().c_str()));

    ACE_Time_Value curTime;
    curTime = curTime.now();
    if(!m_FirstSendMsgFlag && SEND_MSG_INTER >= (curTime - m_MsgTimer).sec())
    {
        return;
    }

    //发送穿透通知消息
    DevID_t sourceId = m_Sess->SourceID();
    DevID_t targetId = m_Sess->TargetID();
    DevObj *source = DevRepo::instance()->GetDevObj(sourceId);
    DevObj *target = DevRepo::instance()->GetDevObj(targetId);
    if(source == NULL || target == NULL || m_Sess->BreakOutFlag())
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Device is null or BreakOuted!\n")));
        m_Next = new TS_TraversalEND(m_Sess);
        return;
    }

    NSClientProxy *sourceNSClient = source->NSProxy();
    NSClientProxy *targetNSClient = target->NSProxy();
    if(sourceNSClient == NULL || targetNSClient == NULL)
    {
        m_Next = new TS_TraversalEND(m_Sess);
        return;
    }
    sourceNSClient->NATTraversalNotify(
            sourceId, theApp.STUNIp(), theApp.STUNPort(), targetId, m_Sess->TraversalSessionID());

    m_MsgTimer = m_MsgTimer.now();
    m_FirstSendMsgFlag = false;
}

void TS_Prepare::HandleMsg(void *msg)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("TS_Prepare HandleMsg Start\n")));
    TraversalStatus::HandleMsg(msg);

    IData data = (*(IData *)msg);

    //穿透会话已存在
    if(data.isMember(KW_RETCODE)
            && RES_CODE_TS_HB == data[KW_RETCODE].asInt()
            && data.isMember(KW_OLDTRAVERSAL_ID)
            )
    {
        SessionId sid(data[KW_OLDTRAVERSAL_ID].asString());
        m_Sess->UpdateSessionIDAttr(sid);
        m_Next = new TS_TraversalHB(m_Sess);
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TS_Prepare HandleMsg into NextStatus[HB]\n")));
    }
    else
    {
        //切换到下一状态
        m_Next = new TS_TraversalReq(m_Sess);
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TS_Prepare HandleMsg into NextStatus[Req]\n")));
    }
}

void TS_TraversalReq::HandleMsg(void *argv)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) TraversalSess Status [Req] HandleMsg!\n")));
    TraversalStatus::HandleMsg(argv);

    m_Data = ((BaseData *)argv)->s_Data;

    if(m_Data.isMember(KW_SUBCMD) && BCSC_RESPONSE == m_Data[KW_SUBCMD].asString()
            && m_Data.isMember(KW_RETCODE))
    {
        int ret = m_Data[KW_RETCODE].asInt();
        if(RES_CODE_OK  == ret)
        {
            m_Next = new TS_TraversalResp(m_Sess);
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TraversalSess Status into next status[Resp]!\n")));
        }
        else
        {
            m_Next = new TS_TraversalEND(m_Sess);
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TraversalSess Status into next status[end], Result[Device return error]!\n")));
        }
        return;
    }

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TraversalSess Status [Req] HandleMsg End!\n")));
    m_ReqFlag = true;
}

void TS_TraversalReq::Work()
{
    ACE_Time_Value curTime;
    curTime = curTime.now();
    if(!m_FirstSendMsgFlag && SEND_MSG_INTER >= (curTime - m_MsgTimer).sec())
    {
        return;
    }

    if(m_ReqFlag)
    {
        DevObj *targetDev = DevRepo::instance()->GetDevObj(m_Sess->TargetID());
        if(targetDev == NULL || m_Sess->BreakOutFlag())
        {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Device is null or BreakOuted!\n")));
            m_Next = new TS_TraversalEND(m_Sess);
            return;
        }
        NSClientProxy *targetNSClient = targetDev->NSProxy();
        if(targetNSClient == NULL)
        {
            m_Next = new TS_TraversalEND(m_Sess);
            return;
        }
        //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) TS_TraversalReq Work3[%s]\n"), m_Data.toStyledString().c_str()));
        targetNSClient->NATTraversalReq(m_Sess->TargetID(), m_Data);
    }

    m_MsgTimer = m_MsgTimer.now();
    m_FirstSendMsgFlag = false;
}

void TS_TraversalResp::HandleMsg(void *argv)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TraversalSess Status [TS_TraversalResp] HandleMsg!\n")));
    TraversalStatus::HandleMsg(argv);

    m_Data = ((BaseData *)argv)->s_Data;

    if(m_Data.isMember(KW_SUBCMD) && BCSC_RESPONSE == m_Data[KW_SUBCMD].asString()
            && m_Data.isMember(KW_RETCODE))
    {
        int ret = m_Data[KW_RETCODE].asInt();
        if(RES_CODE_OK  == ret)
        {
            m_Next = new TS_TraversalWait(m_Sess);
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TraversalSess Status into next status[Wait]!\n")));
        }
        else
        {
            m_Next = new TS_TraversalEND(m_Sess);
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TraversalSess Status into next status[end], Result[Device return error]!\n")));
        }
        return;
    }

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) TraversalSess Status [Resp] HandleMsg End!\n")));
    m_ReqFlag = true;
}

void TS_TraversalResp::Work()
{
    ACE_Time_Value curTime;
    curTime = curTime.now();
    if(!m_FirstSendMsgFlag && SEND_MSG_INTER >= (curTime - m_MsgTimer).sec())
    {
        return;
    }

    if(m_ReqFlag)
    {
        DevObj *sourceDev = DevRepo::instance()->GetDevObj(m_Sess->SourceID());
        if(sourceDev == NULL || m_Sess->BreakOutFlag())
        {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Device is null or BreakOuted!\n")));
            m_Next = new TS_TraversalWait(m_Sess);
            return;
        }
        NSClientProxy *targetNSClient = sourceDev->NSProxy();
        if(targetNSClient == NULL)
        {
            m_Next = new TS_TraversalEND(m_Sess);
            return;
        }
        targetNSClient->NATTraversalResp(m_Sess->SourceID(), m_Data);
    }

    m_MsgTimer = m_MsgTimer.now();
    m_FirstSendMsgFlag = false;
}

void TS_TraversalWait::HandleMsg(void *argv)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) TraversalSess Status [TS_TraversalWait] HandleMsg!\n")));
    TraversalStatus::HandleMsg(argv);

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) TraversalSess Status [Result] HandleMsg End!\n")));
}

void TS_TraversalHB::HandleMsg(void *argv)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) TraversalSess Status [TS_TraversalHB] HandleMsg!\n")));
    TraversalStatus::HandleMsg(argv);

    //接收到TraversalRetry消息，重新创建会话
    IData data = ((BaseData *)argv)->s_Data;
    if(NSC_PT_NODESTRAVERSALRETRY == data[KW_SUBCMD].asString())
    {
        //创建新会话
        m_Sess->RecreateNewSession();

        //发送穿透停止消息
#if 0
        DevID_t sourceId = m_Sess->SourceID();
        DevID_t targetId = m_Sess->TargetID();
        DevObj *source = DevRepo::instance()->GetDevObj(sourceId);
        DevObj *target = DevRepo::instance()->GetDevObj(targetId);
        if(source == NULL || target == NULL || m_Sess->BreakOutFlag())
        {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) Device is null or BreakOuted!\n")));
            m_Next = new TS_TraversalEND(m_Sess);
            return;
        }

        NSClientProxy *sourceNSClient = source->NSProxy();
        NSClientProxy *targetNSClient = target->NSProxy();
        if(sourceNSClient != NULL)
        {
            sourceNSClient->NATTraversalStop(sourceId, m_Sess->TraversalSessionID().tostr());
        }
        if(targetNSClient != NULL)
        {
            targetNSClient->NATTraversalStop(targetId, m_Sess->TraversalSessionID().tostr());
        }
#endif

        //结束本会话
        m_Next = new TS_TraversalEND(m_Sess);
        return;
    }

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TraversalSess Status [Result] HandleMsg End!\n")));
}

void TS_TraversalEND::HandleMsg(void *argv)
{
    TraversalStatus::HandleMsg(argv);
}

void TS_TraversalEND::Work()
{
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TS_End[%s] Start\n"), m_Sess->TraversalSessionID().tostr().c_str()));

    ACE_Time_Value curTime;
    curTime = curTime.now();
    if(!m_FirstSendMsgFlag && SEND_MSG_INTER >= (curTime - m_MsgTimer).sec())
    {
        return;
    }

    //发送穿透通知消息
    DevID_t sourceId = m_Sess->SourceID();
    DevID_t targetId = m_Sess->TargetID();
    DevObj *source = DevRepo::instance()->GetDevObj(sourceId);
    DevObj *target = DevRepo::instance()->GetDevObj(targetId);
    if(source == NULL || target == NULL)
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Device is null!\n")));
        m_Next = new TS_TraversalDie(m_Sess);
        return;
    }

    NSClientProxy *sourceNSClient = source->NSProxy();
    NSClientProxy *targetNSClient = target->NSProxy();
    if(sourceNSClient != NULL)
    {
        sourceNSClient->NATTraversalStop(sourceId, m_Sess->TraversalSessionID().tostr());
    }
    if( targetNSClient != NULL)
    {
        targetNSClient->NATTraversalStop(targetId, m_Sess->TraversalSessionID().tostr());
    }

    m_MsgTimer = m_MsgTimer.now();
    m_FirstSendMsgFlag = false;

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TS_End[%s] End\n"), m_Sess->TraversalSessionID().tostr().c_str()));
}
