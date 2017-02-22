#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "ace/Time_Value.h"
#include "PSUnits.h"
#include "ClientProxy.h"
#include "TraversalSession.h"

class DevObj 
{
    public:
        DevObj(DevID_t devID) : m_DevID(devID), m_DevStatus(OFFLINE), m_NSClientProxy(NULL)
        {
            m_UpdateTimer = m_UpdateTimer.now();
        }
        
        /**
         * @brief 设备状态
         *  OFFLINE：离线；ONLINE：在线（且空闲）；BUSY：忙（处于穿透过程中）
         */
        typedef enum DevStatus_t{OFFLINE=0, ONLINE, BUSY} DevStatus;

        DevID_t DevObjID()
        {
            return m_DevID;
        }

        NSClientProxy *NSProxy()
        {
            return m_NSClientProxy;
        }

        void NSProxy(NSClientProxy *proxy)
        {
            m_NSClientProxy = proxy;
        }
        
        /**
         * @brief IsOnLine 是否在线（包括空闲、忙)
         *
         * @return 
         */
        bool IsOnLine()
        {
            return OFFLINE != m_DevStatus;
        }

        /**
         * @brief IsFree 是否空闲
         *
         * @return 
         */
        bool IsFree()
        {
            return ONLINE == m_DevStatus; 
        }

        bool IsBusy()
        {
            return BUSY == m_DevStatus;
        }

        bool IsOffline()
        {
            return OFFLINE == m_DevStatus;
        }

        ACE_RW_Thread_Mutex *DevStatusMutex()
        {
            return &m_StatusMutex;
        }

        void DeviceStatus(DevStatus_t status)
        {
            ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_StatusMutex);
            m_DevStatus = status;
        }

        void UpdateDevTimer()
        {
            m_UpdateTimer = m_UpdateTimer.now();
        }

        void DevBandWidth(int bandWidth)
        {
            m_DevReBandWidth = bandWidth;
        }

        int DevBandWidth()
        {
            return m_DevReBandWidth;
        }

        TraversalSessionRepo::SessionVec_t& TraversalSession()
        {
            return m_TraversalSessions;
        }

        void DevOnLine();
        void DevOffLine();
        bool UpdateDevStatus(ACE_INET_Addr &peer, int trmStatus);
        void PushSession(TraversalSessionObj *sess);
        bool PopSession(TraversalSessionObj *sess);

    private:
        DevID_t                 m_DevID;
        DevStatus               m_DevStatus;
        ACE_RW_Thread_Mutex     m_StatusMutex;
        int                     m_DevReBandWidth;         //设备剩余带宽
        //int                     m_DevPerLoad;              //负载百分比
        NSClientProxy           *m_NSClientProxy;
        ACE_Time_Value          m_UpdateTimer;
        TraversalSessionRepo::SessionVec_t  m_TraversalSessions;
        friend class DevRepo;

};

class DevRepo : public PSThread
{
    public:
        DevRepo() : PSThread("DevRepo") {}
        static DevRepo *instance();
        virtual int svc(void);

        /**
         * @brief GetDevObj 获取设备对象
         *
         * @param devID 设备ID
         * @param IsCreateFlag 是否创建新设备对象（在无此设备对象情况下）
         *
         * @return 找到设备对象或新创建的设备对象（未找到为NULL）
         */
        DevObj *GetDevObj(DevID_t devID, bool IsCreateFlag = false);

        typedef std::map<DevID_t, DevObj *> DevRepo_t;
    private:
        DevRepo_t               m_DevRepo;
        ACE_RW_Thread_Mutex     m_DevRepo_Mutex;
};

#endif
