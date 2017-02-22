#include "ProtocolLib/JBody.h"
#include "ProtocolLib/Head.h"
#include "PSUnits.h"
#include "ClientHandler.h"
#include "MsgHandler.h"

int ClientHandler::open(void *p)
{
    if(super::open(p) == -1)
    {
        return -1;
    }

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Connection start\n")));
    //ACE_Time_Value iter_delay(30);
    //return this->reactor()->schedule_timer(this, 0, ACE_Time_Value::zero, iter_delay);

    return 0;
}

#if 0
int BCSClientHandler::close(u_long flags)
{
    return 0;
}
#endif

int ClientHandler::handle_input(ACE_HANDLE)
{
    unsigned char buffer[MSG_BUF_SIZE] = {0};
    int recvCnt = this->peer().recv(buffer, PRO_HEAD_SIZE);
    if(recvCnt <= 0)
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Connection close\n")));
        this->reactor()->end_reactor_event_loop();
        return -1;
    }

    Head head;
    try
    {
        head.Parser(buffer);
    }
    catch(int e)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Parse msg failed[%d]!\n"), e), -1);
    }

    uint16_t length = head.Length();
    memset(buffer, 0, MSG_BUF_SIZE);
    recvCnt = this->peer().recv((void *)buffer, length);
    if(recvCnt <= 0)
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Connection close\n")));
        this->reactor()->end_reactor_event_loop();
        return -1;
    }
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Recv msg[%s]\n"), buffer));
    return this->HandleMessage(buffer, recvCnt); 
}

/**
 * @brief SendMsg 发送消息
 *
 * @param msg 待发送的消息
 * @param len 消息长度
 *
 * @return  发送字节数
 */
int ClientHandler::SendMsg(const char *msg, size_t len)
{
    ssize_t sendCnt = this->peer().send(msg, len);
    if(sendCnt <= 0)
    {
        ACE_ERROR((LM_ERROR, ACE_TEXT("[%D](%P|%t) Send msg failed\n")));
        return -1;
    }
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Send msg succ[%d]: %s\n"), sendCnt, msg + 16));

    return sendCnt;
}

int ClientHandler::handle_timeout(const ACE_Time_Value &, const void *)
{
#if 0
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) handle_timeout\n")));
    const int INPUTSIZE= 4096;
    char buff[INPUTSIZE] = {0};
    int nbytes = ACE_OS::sprintf(buff, "HeartBeat\n", "");
    this->peer().send(buff, nbytes);
#endif

    return 0;
}

int ClientHandler::HandleMessage(const unsigned char *buff, int size)
{
    JBody jb(buff, size);
    MsgHandler *handler = MsgHandlerFactory::instance()->GetMsgHandler(jb);
    if(handler == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("[%T](%P|%t) Invalid msg cmd!\n")), -1);
    }
    handler->HandleMsg((void *)&jb);
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Handle Message end\n")));
}

