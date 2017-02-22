#include "Device.h"
#include "TraversalSession.h"
#include "TraversalStatus.h"

static TraversalSessionRepo _ins_TraversalSessionRepo;

TraversalSessionRepo *TraversalSessionRepo::instance()
{
    return (&_ins_TraversalSessionRepo);
}

int TraversalSessionRepo::svc(void)
{
    while(1)
    {
        ACE_OS::sleep(ACE_Time_Value(0, 50));
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("TraversalSessionRepo Start\n")));
        ScanWorkingSess();
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("TraversalSessionRepo End\n")));
    }
}

int TraversalSessionRepo::ScanWorkingSess()
{
    //todo:
    //清理过期会话(记得更新TopologyInfo中的会话关系)
    //执行会话的工作程序
#if 0
    if(m_ShowTimes != 5000)
    {
        m_ShowTimes++;
    }
    else
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) ScanWorkSess %d\n"), m_WorkingSessions.size()));
        m_ShowTimes = 0;
    }
#endif

    TS_Value statusValue;
    SessionMap_t::iterator it = m_WorkingSessions.begin();
    while(it != m_WorkingSessions.end())
    {
        SessionVec_t sessObjList = it->second;
        SessionVec_t::iterator sit = sessObjList.begin();
        while(sit != sessObjList.end())
        {
            statusValue = (*sit)->StatusValue();
            if(TS_DIE == statusValue)
            {
                TraversalSessionObj *sess = *sit;
                sit = sessObjList.erase(sit);
                sess->IntoSessionUnTraversal();
                delete sess;
                continue;
            }
            else if(TS_HB == statusValue)
            {
                sit++;
                continue;
            }
            (*sit)->Work();
            sit++;
        }

        if(sessObjList.empty())
        {
            m_WorkingSessions.erase(it);
        }
        it++;
    }
}

TraversalSessionObj *TraversalSessionRepo::GetSessionObj(const SessionId &id)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("GetSessionObj Start \n")));
    unsigned int hid = id.hash_id();
    TraversalSessionObj *sess = this->GetSession(hid, id);
    if(sess == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get session[%s] failed!\n"), id.tostr().c_str()), NULL);
    }

    //todo: 会话状态正常（是活的），添加会话的引用

    return sess;
}

TraversalSessionObj *TraversalSessionRepo::GetSession(unsigned int hashId, const SessionId &id)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("GetSession(2) Start \n")));
    TraversalSessionObj *sess = this->GetSession(m_WorkingSessions, hashId, id);

    return sess;
}

TraversalSessionObj *TraversalSessionRepo::GetSession(SessionMap_t &repo, unsigned int hashId, const SessionId& id)
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("GetSession(3) Start \n")));
    SessionMap_t::iterator it = repo.find(hashId);
    if(it == repo.end())
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get SessionObj[%s] failed!\n"), id.tostr().c_str()), NULL);
    }

    SessionVec_t& sessions = it->second;
    SessionVec_t::iterator vit = sessions.begin();
    while(vit != sessions.end())
    {
        if((*vit)->TraversalSessionID() == id)
        {
            return (*vit);
        }
        else
        {
            vit++;
        }
    }
    return NULL;
}

/**
 * @brief CreateSessionObj 创建穿透会话
 * 1、判断是否允许创建会话（条件：终端存在，且在线）
 * 2、创建会话（INIT状态，当终端空闲时，转入Prepare状态）
 *
 * @param sourceId
 * @param targetId
 *
 * @return 
 */
bool TraversalSessionRepo::CreateSessionObj(DevID_t sourceId, DevID_t targetId)
{
    DevObj *source = DevRepo::instance()->GetDevObj(sourceId);
    DevObj *target = DevRepo::instance()->GetDevObj(targetId);
    if(source == NULL || target == NULL)
    {
        return false;
    }

    ACE_RW_Thread_Mutex *tmpSourceMutex = source->DevStatusMutex();
    ACE_RW_Thread_Mutex *tmpTargetMutex = target->DevStatusMutex();

    ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(*tmpSourceMutex);
    ACE_Read_Guard<ACE_RW_Thread_Mutex> guard2(*tmpTargetMutex);
    if(source->IsOffline() || target->IsOffline())
    {
        return false;
    }

    string id = this->GenSessionId();
    TraversalSessionObj *sess = new TraversalSessionObj(id, sourceId, targetId); 
    if(sess == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Create SessionObj Failed!\n")), false);
    }
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%T](%P|%t) Create Session succ!\n")));
    unsigned int hid = sess->TraversalSessionID().hash_id();
    m_WorkingSessions[hid].push_back(sess);

    source->PushSession(sess);
    target->PushSession(sess);
}

int TraversalSessionRepo::UpdateSessionObjBySid(SessionId& oldSid, SessionId& newSid)
{
    TraversalSessionObj *sess = NULL;
    unsigned int oldHid = oldSid.hash_id();
    SessionMap_t::iterator it = m_WorkingSessions.find(oldHid);
    if(it != m_WorkingSessions.end())
    {
        SessionVec_t::iterator vit = it->second.begin();
        while(vit != it->second.end())
        {
            if(oldSid == (*vit)->TraversalSessionID())
            {
                sess = *vit;
                break;
            }
        }
        it->second.erase(vit);

        if(it->second.empty())
        {
            m_WorkingSessions.erase(it);
        }
    }

    if(sess == NULL)
    {
        ACE_ERROR_RETURN((LM_ERROR, 
                    ACE_TEXT("Update session object failed!Can't found the sessObj!\n")), 
                -1);
    }

    unsigned int newHid = newSid.hash_id();
    m_WorkingSessions[newHid].push_back(sess);
}

bool TraversalSessionRepo::RemoveSessionObj(DevID_t sourceId, DevID_t targetId)
{
    DevObj *dev = DevRepo::instance()->GetDevObj(sourceId);
    if(NULL == dev)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Get device failed[RemoveSession]!\n")), -1);
    }
    SessionVec_t sessList = dev->TraversalSession(); 
    SessionVec_t::iterator it = sessList.begin();
    while(it != sessList.end())
    {
        if(targetId == (*it)->TargetID())
        {
            (*it)->BreakOutFlag(true);
        }
        it++;
    }
}

string TraversalSessionRepo::GenSessionId()
{
    ACE_Utils::UUID uuid;
    m_UUIDGener.generate_UUID(uuid);

    string id = uuid.to_string()->c_str();
    string::iterator it = id.erase(id.begin() + 8);
    it = id.erase(it + 4);
    it = id.erase(it + 4);
    it = id.erase(it + 4);

    return id;
}

TraversalSessionObj::TraversalSessionObj(string &id, DevID_t source, DevID_t target) 
            : m_SID(id), m_Source(source), m_Target(target), m_BreakOutFlag(false) 
{
    m_Status = new TS_TraversalInit(this);
}

TraversalSessionObj::~TraversalSessionObj()
{
}

TS_Value TraversalSessionObj::StatusValue()
{
    return m_Status->StatusValue();
}

void TraversalSessionObj::Work()
{
    //ACE_DEBUG((LM_DEBUG, ACE_TEXT("TraversalSessionObj Work[%d]\n"), m_Status->StatusValue()));
    //执行status的work进程
    TraversalStatus *newStatus = m_Status->Next();
    if(newStatus != NULL)
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) TraversalSessionObj WorkNext \n"), m_Status->StatusValue()));
        delete m_Status;
        m_Status = newStatus;
    }

    m_Status->Work();
}

int TraversalSessionObj::HandleMsg(void *argv)
{
    m_Status->HandleMsg(argv);
}

void TraversalSessionObj::UpdateSessionIDAttr(SessionId &sid)
{
    TraversalSessionRepo::instance()->UpdateSessionObjBySid(m_SID, sid);
    m_SID = sid;
}

//会话正在穿透过程中
bool TraversalSessionObj::IntoSessionTraversaling()
{
    DevObj *source = DevRepo::instance()->GetDevObj(m_Source);
    DevObj *target = DevRepo::instance()->GetDevObj(m_Target);
    if(source == NULL || target == NULL)
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Device is null or BreakOuted!\n")));
        return false;
    }
    source->DeviceStatus(DevObj::BUSY);
    target->DeviceStatus(DevObj::BUSY);

    return true;
}

bool TraversalSessionObj::IntoSessionUnTraversal()
{
    return IntoSessionTraversaled();
}

//会话穿透完成
bool TraversalSessionObj::IntoSessionTraversaled()
{
    DevObj *source = DevRepo::instance()->GetDevObj(m_Source);
    DevObj *target = DevRepo::instance()->GetDevObj(m_Target);
    if(source == NULL || target == NULL)
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) Device is null or BreakOuted!\n")));
        return false;
    }
    source->DeviceStatus(DevObj::ONLINE);
    target->DeviceStatus(DevObj::ONLINE);

    return true;
}

bool TraversalSessionObj::RecreateNewSession()
{
    TraversalSessionRepo::instance()->CreateSessionObj(m_Source, m_Target);
}
