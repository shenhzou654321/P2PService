#include "Device.h"
#include "TopologyInfo.h"

static DevRepo _ins_DevRepo;

DevRepo *DevRepo::instance()
{
    return (&_ins_DevRepo);
}

DevObj *DevRepo::GetDevObj(DevID_t devID, bool IsCreateFlag)
{
    DevObj *devObj = NULL;
    {
        ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_DevRepo_Mutex);
        DevRepo_t::iterator it;
        it = m_DevRepo.find(devID);
        if(it != m_DevRepo.end())
        {
            devObj = it->second;
            return devObj;
        }
        if(!IsCreateFlag)
        {
            return NULL;
        }
    }

    if(IsCreateFlag)
    {
        ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_DevRepo_Mutex);
        devObj = new DevObj(devID);
        if(devObj == NULL)
        {
            ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("[%D](%P|%t) Create DevObj[%d] failed!\n"), devID), NULL);
        }
        m_DevRepo.insert(
                std::pair<DevID_t, DevObj *>(devID, devObj));
    }

    return devObj;
}

int DevRepo::svc(void)
{
    ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_DevRepo_Mutex);
    ACE_Time_Value curTimer;
    curTimer = curTimer.now();
    DevRepo_t::iterator it = m_DevRepo.begin();
    DevObj *curDev = NULL;
    //终端上下线，将影响穿透会话
    while(it != m_DevRepo.end())
    {
        curDev = it->second;
        if(curDev->IsOffline() 
            && (DEV_OFFLINE_TIMEOUT <= (curTimer - curDev->m_UpdateTimer).sec()))
        {
            m_DevRepo.erase(it);
            delete curDev;
        }
        else if(curDev->IsOnLine() 
                && (SCAN_TRM_ONLINE_INTER <= (curTimer - curDev->m_UpdateTimer).sec()))
        {
        }
        else
        {
            it++;
        }
    }
}

void DevObj::DevOnLine()
{
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t)Device[%d] online\n"), m_DevID));
    DevObj *targetDev = NULL;
    DevsVector_t devs = TopologyInfo::instance()->GetAllTargetDevs(m_DevID);
    DevRepo *repo = DevRepo::instance();

    DevsVector_t::iterator it = devs.begin();
    while(it != devs.end())
    {
        targetDev = repo->GetDevObj(*it);
        if(targetDev != NULL && targetDev->IsOnLine())
        {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t)Device[%d]: TraversalTargetDevice[%d] online\n"), m_DevID, targetDev->DevObjID()));
        }
        else if(targetDev != NULL)
        {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t)Device[%d]: TraversalTargetDevice[%d] Offline)\n"), m_DevID, targetDev->DevObjID()));
        }
        else
        {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t)Device[%d]: TraversalTargetDevice[%d] Can't found!\n"), m_DevID, (*it)));
        }

        if(targetDev != NULL && targetDev->IsOnLine())
        {
            TraversalSessionRepo::instance()->CreateSessionObj(m_DevID, *it);
        }
        it++;
    }
}

void DevObj::DevOffLine()
{
    TraversalSessionRepo::SessionVec_t::iterator it = m_TraversalSessions.begin();
    while(it != m_TraversalSessions.end())
    {
        (*it)->BreakOutFlag(true);
        it++;
    }
}

bool DevObj::UpdateDevStatus(ACE_INET_Addr &peer, int trmStatus)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("UpdateDevInfo Start\n")));
    m_NSClientProxy = NSClientRepo::instance()->UpdateNSInfo(this->m_DevID, peer);
    if(NULL == m_NSClientProxy)
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) UpdateDevInfo: NSClientProxy is NULL\n")));
        return false;
    }

    DevStatus tmpStatus = m_DevStatus; 
    //所有非离线状态，都认为在线
    if(OFFLINE != trmStatus)
    {
        m_DevStatus = ONLINE;   //空闲
    }
    else
    {
        m_DevStatus = OFFLINE;  //离线
    }

    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("UpdateDevInfo: tmpStatus[%d] m_DevStatus[%d][%d]\n"), tmpStatus, m_DevStatus, trmStatus));
    //掉线（在线/忙-->离线）
    if(tmpStatus != m_DevStatus && m_DevStatus == OFFLINE)
    {
        this->DevOffLine();
    }
    //上线
    else if(tmpStatus != m_DevStatus && tmpStatus == OFFLINE)
    {
        this->DevOnLine();
    }

    UpdateDevTimer();
    return true;
}

void DevObj::PushSession(TraversalSessionObj *sess)
{
    m_TraversalSessions.push_back(sess);
}

bool DevObj::PopSession(TraversalSessionObj *sess)
{
    TraversalSessionRepo::SessionVec_t::iterator it =
        m_TraversalSessions.begin();
    while(it != m_TraversalSessions.end())
    {
        if(sess == *it)
        {
            m_TraversalSessions.erase(it);
            return true;
        }
        it++;
    }

    return false;
}
