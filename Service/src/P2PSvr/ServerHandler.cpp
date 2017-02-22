#include "MsgHandler.h"
#include "ProtocolLib/Head.h"
#include "ProtocolLib/JBody.h"
#include "TCPHandler.h"

int ServerHandler::open(void *p)
{
    if(super::open(p) == -1)
    {
        return -1;
    }

    ACE_INET_Addr peer_addr;
    if(this->peer().get_remote_addr(peer_addr) == 0
                && peer_addr.addr_to_string(peer_name, MAXHOSTNAMELEN) == 0)
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Connection from %s\n"), peer_name));
    }

    //this->notifier.reactor(this->reactor());
    //this->msg_queue()->notification_strategy(&this->notifier);

    //return this->activate();
    return 0;
}

int ServerHandler::close(u_long flags)
{
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t)Handler clase\n")));
    return 0;
}

int ServerHandler::handle_input(ACE_HANDLE)
{
    unsigned char buff[MSG_BUF_SIZE] = {0}; 

    //接收包头
    int recvCnt = this->peer().recv(buff, PRO_HEAD_SIZE);
    if(recvCnt <= 0)
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Connection close from %s\n"), peer_name));
        return -1;
    }

    Head head;
    try
    {
        head.Parser(buff);
    }
    catch(...)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Parse msg failed!\n")), -1);
    }

    uint16_t length = head.Length();
    memset(buff, 0, MSG_BUF_SIZE);
    recvCnt = this->peer().recv((void *)buff, length); 
    if(recvCnt <= 0)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("[%D](%P|%t) Recv msg body failed\n")), -1);
    }

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Recv msg[%s]\n"), buff));
    this->HandleMessage(buff, length);
}

int ServerHandler::HandleMessage(const unsigned char *buff, int size)
{
    JBody jb(buff, size);
    MsgHandler *handler = MsgHandlerFactory::instance()->GetMsgHandler(jb);
    if(handler == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("[%D](%P|%t) Invalid msg cmd!\n")), -1);
    }
    handler->HandleMsg((void *)&jb, this);
}
