/*************************************************************************
    > File Name: devrepo.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2016年01月04日 星期一 16时02分47秒
 ************************************************************************/

#ifndef _RELAYNODE_DEVREPO_H_
#define _RELAYNODE_DEVREPO_H_

#include "device.h"
#include "spinlock.h"
#include <map>

using std::map;

class DevRepo
{
public:
	~DevRepo();

	static DevRepo* Instance();
	static void Destroy();

	Device* AddDevice(devid_t devid);
	bool RmDevice(devid_t devid);
	Device* GetDevice(devid_t devid);
	void OnTimer();

private:
	DevRepo();
	void Cleanup();

protected:
	static DevRepo* m_pInst;
	typedef map<devid_t, Device*> dev_map_t;
	dev_map_t m_devMap;
	SpinLock m_lock;
};

#endif // _RELAYNODE_DEVREPO_H_


