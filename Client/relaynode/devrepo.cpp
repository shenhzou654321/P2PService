/*************************************************************************
    > File Name: devrepo.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2016年01月04日 星期一 16时07分42秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "devrepo.h"


DevRepo* DevRepo::m_pInst = NULL;

DevRepo::DevRepo()
{

}

DevRepo::~DevRepo()
{

}

DevRepo* DevRepo::Instance()
{
	if (m_pInst == NULL)
	{
		m_pInst = new DevRepo();
	}
	return m_pInst;
}

void DevRepo::Destroy()
{
	if (m_pInst != NULL)
	{
		delete m_pInst;
		m_pInst = NULL;
	}
}

Device* DevRepo::AddDevice(devid_t devid)
{
	Device* pdevice = GetDevice(devid);
	if (pdevice == NULL)
	{
		pdevice = new Device(devid);
		MyLock lock(&m_lock);
		lock.SetBusy();
		m_devMap[devid] = pdevice;
	}
	return pdevice;
}

bool DevRepo::RmDevice(devid_t devid)
{
	dev_map_t::iterator iter = m_devMap.find(devid);
	if (iter != m_devMap.end())
	{
		Device* pdevice = iter->second;
		MyLock lock(&m_lock);
		lock.SetBusy();
		m_devMap.erase(iter);

		delete pdevice;
	}
	return true;
}

Device* DevRepo::GetDevice(devid_t devid)
{
	dev_map_t::iterator iter = m_devMap.find(devid);
	if (iter != m_devMap.end())
	{
		Device* pdevice = iter->second;
		return pdevice;
	}
	return NULL;
}


void DevRepo::Cleanup()
{
	dev_map_t devMap;
	dev_map_t::iterator iter;

	devMap = m_devMap;
	m_devMap.clear();

	for (iter = devMap.begin(); iter != devMap.end(); iter++)
	{
		Device* pdevice = iter->second;
		delete pdevice;
	}
	devMap.clear();	
}

void DevRepo::OnTimer()
{
	dev_map_t::iterator iter;

	MyLock lock(&m_lock);
	if (!lock.TestAndSetBusy())
	{
		return;
	}

	for (iter = m_devMap.begin(); iter != m_devMap.end(); iter++)
	{
		Device* pdevice = iter->second;
		pdevice->OnTimer();
	}
}

