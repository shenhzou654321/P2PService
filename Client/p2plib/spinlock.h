/*************************************************************************
    > File Name: spinlock.h
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年12月08日 星期二 09时59分01秒
 ************************************************************************/

#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include <pthread.h>
#include <stdio.h>
#include <assert.h>

class SpinLock
{
public:
	SpinLock()
	{
		int ret = pthread_spin_init(&m_lock, 0);
		if (ret != 0)
		{
			printf("pthread_spin_init failed");
		}
	}

	~SpinLock()
	{
		pthread_spin_destroy(&m_lock);
	}

	int TryAcquire()
	{
		return pthread_spin_trylock(&m_lock);
	}

	int Acquire()
	{
		return pthread_spin_lock(&m_lock);
	}

	int Release()
	{
		return pthread_spin_unlock(&m_lock);
	}

private:
	pthread_spinlock_t m_lock;
};

class MyLock
{
public:
	MyLock(SpinLock* lock)
	{
		assert(lock != NULL);
		m_lock = lock;
		m_locked = false;
	}
	~MyLock()
	{
		m_lock->Release();
	}
	bool TestAndSetBusy()
	{
		m_locked = m_lock->TryAcquire() == 0;
		return m_locked;
	}
	void SetBusy()
	{
		m_lock->Acquire();
		m_locked = true;
	}

private:
	MyLock()
	{
		;
	}

protected:
	bool m_locked;
	SpinLock* m_lock;
};

#endif // _SPINLOCK_H_

