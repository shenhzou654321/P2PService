#include "ClientProxy.h"
#include "Device.h"

static NSClientRepo _ins_NSClientRepo;

NSClientRepo *NSClientRepo::instance()
{
    return (&_ins_NSClientRepo);
}

/**
 * @brief UpdateNSInfo 更新NS信息 
 * 1、更新NS列表
 * 2、更新设备与NS对应列表
 */
NSClientProxy *NSClientRepo::UpdateNSInfo(DevID_t devId, ACE_INET_Addr &ipAddr)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) UpdateNSInfo\n")));

    ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_Mutex);
    NSClientProxy *newProxy = NULL;
    uint32_t ip = ipAddr.get_ip_address();
    IPNSClientRepo_t::iterator it = m_NSClientRepo->find(ip);
    if(it != m_NSClientRepo->end())
    {
        //it->second->UpdateTimer();
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t)Get NSProxy succ\n")));
        newProxy = it->second;
    }
    else
    {
        ACE_ERROR_RETURN((LM_ERROR, 
                    ACE_TEXT("[%D](%P|%t) Invalid NSService:%s:%d\n"), 
                    ipAddr.get_host_name(), 
                    ipAddr.get_port_number()), NULL);
        //return NULL;
    }

    DevObj *dev = DevRepo::instance()->GetDevObj(devId);
    if(dev == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("[%D](%P|%t) Get Device[%d] failed!\n"), devId), NULL);
    }
    dev->NSProxy(newProxy);
    return newProxy;
}

int NSClientRepo::UpdateNSProxys(std::vector<ACE_INET_Addr> list)
{
    ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_Mutex);
    std::vector<ACE_INET_Addr>::iterator it = list.begin();
    IPNSClientRepo_t *clientRepo = new IPNSClientRepo_t();
    IPNSClientRepo_t::iterator nit; 

    while(it != list.end())
    {
        uint32_t tmpIP = it->get_ip_address();
        //NS Client集合不存在（如服务器启动时）或NS Client集需要更新时（如BCS通知：NSSvr配置信息有更新 ）
        if(m_NSClientRepo == NULL               
                || (nit = m_NSClientRepo->find(tmpIP)) == m_NSClientRepo->end())         
        {
            //新的NSClient
            NSClientProxy *newProxy = new NSClientProxy();
            if(newProxy == NULL)
            {
                ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("[%D](%P|%t) Create NSClientProxy failed!\n")), -1);
            }
            else
            {
                char ipStr[MAXHOSTNAMELEN] = {0};
                it->addr_to_string(ipStr, MAXHOSTNAMELEN);
                ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Create NSClientProxy[%s] succ!\n"), ipStr));
            }

            newProxy->Init(*it);
            newProxy->Start();
            clientRepo->insert(
                    std::pair<uint32_t, NSClientProxy *>(tmpIP, newProxy));
        }
        else
        {
            clientRepo->insert(*nit);
            m_NSClientRepo->erase(nit);
        }

        it++;
    }

    if(m_NSClientRepo == NULL)
    {
        m_NSClientRepo = clientRepo;
        return 0;
    }

    //清理旧的NSClient
    IPNSClientRepo_t::iterator rit = m_NSClientRepo->begin();
    while(rit != m_NSClientRepo->end())
    {
#if 0
        //清理设备-NS客户端对应表
        ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_DO_Mutex);
        CleanDevOwners(rit->second);
#endif

        //清理NS服务器对象本身
        delete rit->second;
    }
    delete m_NSClientRepo;
    m_NSClientRepo = clientRepo;

    return 0;
}
