#ifndef __TOPOLOGYINFO_H__
#define __TOPOLOGYINFO_H__
#include <vector>
#include <map>
#include "Device.h"
#include "TraversalSession.h"
using std::map;
using std::vector;


/**
 * @brief 设备穿透的拓扑结构类
 */
class TopologyInfo : public PSThread
{
    public:
        typedef struct SingleTopologyStruct_t
        {
            DevID_t             s_ChildDev;
            DevID_t             s_FatherDev;
            //TraversalSessionObj *s_Sess;

            SingleTopologyStruct_t(DevID_t childDev, DevID_t fatherDev) : 
                s_ChildDev(childDev), s_FatherDev(fatherDev)//, s_Sess(NULL)
            {}
        } SingleTopologyStruct;
        typedef map<DevID_t, SingleTopologyStruct *> SonFatherMap_t ;
        typedef map<DevID_t, DevsVector_t> FatherSonMap_t;
        typedef enum Oper {NEW=0, UPDATE, DEL} OPER;
        typedef struct DevChangeStruct_t
        {
            SingleTopologyStruct        *s_OldInfo;     //旧数据
            SingleTopologyStruct        *s_TopoInfo;    //新数据
            OPER                        s_Oper;

            DevChangeStruct_t() : s_OldInfo(NULL), s_TopoInfo(NULL) 
            {}
        } DevChangeStruct;

        TopologyInfo() : PSThread("TopologyInfo"), m_SonList(NULL), m_FatherList(NULL) {}
        static TopologyInfo *instance();
        virtual int svc(void);

        /**
         * @brief FillInTopologyInfo 填充拓扑信息
         *
         * @param map 
         */
        void FillInTopologyInfo(const FatherSonMap_t &map);

        /**
         * @brief GetTargetDevs 获取指定设备的目标穿透设备列表
         *
         * @param devID
         *
         * @return 
         */
        DevsVector_t GetTargetDevs(DevID_t devID);
        DevsVector_t GetAllTargetDevs(DevID_t devID);

    private:
        SonFatherMap_t  *m_SonList;      //一对一关系
        FatherSonMap_t  *m_FatherList;   //一对多关系
        vector<DevChangeStruct> m_ChangeList;
};

#endif  //__TOPOLOGYINFO_H__
