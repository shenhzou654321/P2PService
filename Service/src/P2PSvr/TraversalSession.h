#ifndef __TRAVERSALSESSION_H__
#define __TRAVERSALSESSION_H__

#include <vector>
#include <map>
#include <ace/UUID.h>
#include "Units.h"
#include "PSThread.h"
#include "PSUnits.h"
//#include "TraversalStatus.h"

using std::vector;
using std::map;

class DevObj;
class TraversalStatus;

class TraversalSessionObj
{
    public:
        TraversalSessionObj(string &id, DevID_t source, DevID_t target) ;
        ~TraversalSessionObj();

        SessionId TraversalSessionID()
        {
            return m_SID;
        }
        DevID_t SourceID()
        {
            return m_Source;
        }
        DevID_t TargetID()
        {
            return m_Target;
        }

        void BreakOutFlag(bool flag)
        {
            m_BreakOutFlag = flag;
        }

        bool BreakOutFlag()
        {
            return m_BreakOutFlag;
        }

        TS_Value StatusValue();

        int HandleMsg(void *);

        /**
         * @brief Work 会话日常工作
         *
         * @param 
         */
        void Work();
        void UpdateSessionIDAttr(SessionId &sid);
        bool IntoSessionTraversaled();
        bool IntoSessionUnTraversal();
        bool IntoSessionTraversaling();
        bool RecreateNewSession();

    private:
        SessionId               m_SID;
        DevID_t                 m_Source;
        DevID_t                 m_Target;
        TraversalStatus         *m_Status;
        bool                    m_BreakOutFlag;         //中断标识
};

class TraversalSessionRepo : public PSThread
{
    public:
        TraversalSessionRepo() : PSThread("TraversalSessionRepo")  {}//m_ShowTimes = 0;}
        static TraversalSessionRepo *instance();
        virtual int svc(void);
        int ScanWorkingSess();
        TraversalSessionObj *GetSessionObj(const SessionId &id);
        bool CreateSessionObj(DevID_t sourceId, DevID_t targetId);
        int UpdateSessionObjBySid(SessionId& oldSid, SessionId& newSid);
        bool RemoveSessionObj(DevID_t sourceId, DevID_t targetId);

        typedef vector<TraversalSessionObj *> SessionVec_t;
        typedef map<unsigned int, SessionVec_t> SessionMap_t;
    private:
        string GenSessionId();
        TraversalSessionObj *GetSession(unsigned int hashId, const SessionId &id);
        TraversalSessionObj *GetSession(SessionMap_t &repo, unsigned int hashId, const SessionId &id);

    private:
        //SessionVec_t                    m_PrepareSessions;      //预处理队列
        SessionMap_t                    m_WorkingSessions;      //
        ACE_Utils::UUID_Generator       m_UUIDGener;            //UUID生成器
        //int                      m_ShowTimes;

};

#endif //__TRAVERSALSESSION_H__
