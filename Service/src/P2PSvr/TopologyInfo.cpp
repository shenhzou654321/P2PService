#include "TopologyInfo.h"

static TopologyInfo _ins_TopologyInfo;

TopologyInfo *TopologyInfo::instance()
{
    return (&_ins_TopologyInfo);
}

/**
 * @brief svc 线程函数 
 * 遍历修改记录列表：处理修改的设备穿透会话
 * 新建：创建穿透会话；更新：发送新指令；删除：发送停止指令到设备会话
 * @return 
 */
int TopologyInfo::svc(void)
{
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("TologyInfo svc start\n")));
    vector<DevChangeStruct>::iterator it = m_ChangeList.begin();
    while(it != m_ChangeList.end())
    {
        SingleTopologyStruct *topoInfo = it->s_TopoInfo;
        SingleTopologyStruct *oldInfo = it->s_OldInfo;
        switch(it->s_Oper)
        {
            case UPDATE:        
            {
                if(topoInfo == NULL || oldInfo == NULL)
                {
                    ACE_ERROR((LM_ERROR, ACE_TEXT("Update topology info failed!\n")));
                    break;
                }
                //删除原Session
                TraversalSessionRepo::instance()->RemoveSessionObj(oldInfo->s_ChildDev, oldInfo->s_FatherDev);
                //新建新的Session
                TraversalSessionRepo::instance()
                    ->CreateSessionObj(topoInfo->s_ChildDev, topoInfo->s_FatherDev);
                break;
            }
                
            case NEW:
            {
                if(topoInfo == NULL)
                {
                    ACE_ERROR((LM_ERROR, ACE_TEXT("New topology info failed!\n")));
                    break;
                }
                //创建新的Session
                TraversalSessionRepo::instance()
                    ->CreateSessionObj(topoInfo->s_ChildDev, topoInfo->s_FatherDev);
                break;
            }

            case DEL:
            {
                //中断会话
                if(oldInfo == NULL)
                {
                    ACE_ERROR((LM_ERROR, ACE_TEXT("Delete topology info failed!\n")));
                    break;
                }
                TraversalSessionRepo::instance()->RemoveSessionObj(oldInfo->s_ChildDev, oldInfo->s_FatherDev);
                break;
            }

            default:
                break;
        }
        if(oldInfo != NULL)
        {
            delete oldInfo;
        }
        it++;
    }

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("TologyInfo svc end\n")));
    m_ChangeList.clear();
}

void TopologyInfo::FillInTopologyInfo(const FatherSonMap_t &map)
{
    m_ChangeList.clear();

    FatherSonMap_t *newList = new FatherSonMap_t(map);
    SonFatherMap_t *newSonsList = new SonFatherMap_t();

    //遍历FatherList（新一对多关系）
    FatherSonMap_t::iterator fit = newList->begin();
    while(fit != newList->end())
    {
        DevsVector_t devList = fit->second;
        //遍历子设备集合
        DevsVector_t::iterator vit = devList.begin();
        while(vit != devList.end())
        {
            SonFatherMap_t::iterator sit;
            SingleTopologyStruct *newTopo = new SingleTopologyStruct(*vit, fit->first);  
            if(m_SonList == NULL 
                    || (sit = m_SonList->find(*vit)) == m_SonList->end())
            {
                //新增穿透关系
                DevChangeStruct changes;
                changes.s_TopoInfo = newTopo;
                changes.s_Oper = NEW;
                m_ChangeList.push_back(changes);
            }
            else 
            {
                //穿透关系有更改，更新
                if(sit->second->s_FatherDev != fit->first)
                {
                    DevChangeStruct changes;
                    changes.s_OldInfo = sit->second;
                    changes.s_TopoInfo = newTopo;
                    changes.s_Oper = UPDATE;
                    m_ChangeList.push_back(changes);
                }
                m_SonList->erase(sit);
            }
            newSonsList->insert(std::pair<DevID_t, SingleTopologyStruct *>(*vit, newTopo));
            //newSonsList[*vit] = fit->first;
            vit++;
        }
        fit++;
    }

    //需要清理的穿透关系（旧）
    if(m_SonList != NULL)
    {
        SonFatherMap_t::iterator dit = m_SonList->begin();
        while(dit != m_SonList->end())
        {
            DevChangeStruct changes;
            changes.s_OldInfo = dit->second;
            changes.s_Oper = DEL;
            m_ChangeList.push_back(changes);
        }
    }

    //todo 考虑增加读写锁
    if(NULL != m_SonList)
    {
        delete m_SonList;
    }
    if(NULL != m_FatherList)
    {
        delete m_FatherList;
    }
    m_SonList = newSonsList;
    m_FatherList = newList;

    //启动线程，处理修改的穿透关系
    this->Start();
}

DevsVector_t TopologyInfo::GetTargetDevs(DevID_t devID)
{
    DevsVector_t devs;
    devs.clear();
    SonFatherMap_t::iterator sit = m_SonList->find(devID);
    if(sit != m_SonList->end())
    {
        devs.push_back(sit->second->s_FatherDev);
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("GetTargetDevs PushBack[%d]\n"), sit->second->s_FatherDev));
    }

    return devs;
}

DevsVector_t TopologyInfo::GetAllTargetDevs(DevID_t devID)
{
    DevsVector_t devs;
    devs = GetTargetDevs(devID);

    FatherSonMap_t::iterator fit = m_FatherList->find(devID);
    if(fit != m_FatherList->end())
    {
        devs.insert(devs.begin(), fit->second.begin(), fit->second.end());
    }
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t)GetAllTargetDevs(%d) devs size[%d]\n"), devID, devs.size()));

    return devs;
}
