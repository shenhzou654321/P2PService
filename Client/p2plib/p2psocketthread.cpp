
#include "commonincludes.hpp"
#include "config.h"
#include "stuncore.h"
#include "p2psocket.h"
#include "p2psocketthread.h"
#include "recvfromex.h"

P2PSocketThread::P2PSocketThread() :
m_fNeedToExit(false),
m_pthread((pthread_t)-1),
m_fThreadIsValid(false),
m_rotation(0)
{
}

P2PSocketThread::~P2PSocketThread()
{
    SignalForStop(true);
    WaitForStopAndClose();
}

void P2PSocketThread::ClearSocketArray()
{
    m_socketList.clear();
}

HRESULT P2PSocketThread::Init()
{
    HRESULT hr = S_OK;
    
    ChkIfA(m_fThreadIsValid, E_UNEXPECTED);
    
    Chk(InitThreadBuffers());

    m_fNeedToExit = false;
    m_rotation = false;

Cleanup:
    return hr;
}

HRESULT P2PSocketThread::InitThreadBuffers()
{
    HRESULT hr = S_OK;
    
    m_spBufferIn = CRefCountedBuffer(new CBuffer(MAX_P2P_MESSAGE_SIZE));

    return hr;
}

void P2PSocketThread::UninitThreadBuffers()
{
    m_spBufferIn.reset();
}

HRESULT P2PSocketThread::Start()
{
    HRESULT hr = S_OK;
    int err = 0;

    ChkIfA(m_fThreadIsValid, E_UNEXPECTED);

    err = ::pthread_create(&m_pthread, NULL, P2PSocketThread::ThreadFunction, this);
 
    ChkIfA(err != 0, ERRNO_TO_HRESULT(err));
    m_fThreadIsValid = true;

Cleanup:
    return hr;
}

HRESULT P2PSocketThread::SignalForStop(bool fPostMessage)
{
	HRESULT hr = S_OK;

	m_fNeedToExit = true;

	if (fPostMessage)
	{
		p2p_socket_list_t tempSocketList;
		tempSocketList = m_socketList;

		for (p2p_socket_list_t::iterator it = tempSocketList.begin();
				it != tempSocketList.end();
				it++)
		{
			char data = 'x';

			P2PSocket* socket = *it;
			CSocketAddress addr(socket->GetLocalAddress());

			if (addr.IsIPAddressZero())
			{
				CSocketAddress addrLocal;
				CSocketAddress::GetLocalHost(addr.GetFamily(), &addrLocal);
				addrLocal.SetPort(addr.GetPort());
				addr = addrLocal;
			}

			::sendto(socket->GetSocketHandle(), &data, 1, 0, addr.GetSockAddr(), addr.GetSockAddrLength());
		}
	}

	return hr;
}

HRESULT P2PSocketThread::WaitForStopAndClose()
{
	void* pRetValFromThread = NULL;

	if (m_fThreadIsValid)
	{
		// now wait for the thread to exit
		pthread_join(m_pthread, &pRetValFromThread);
	}

	m_fThreadIsValid = false;
	m_pthread = (pthread_t)-1;

	ClearSocketArray();

	UninitThreadBuffers();

	return S_OK;
}

void* P2PSocketThread::ThreadFunction(void* pThis)
{
	((P2PSocketThread*)pThis)->Run();
	return NULL;
}

P2PSocket* P2PSocketThread::WaitForSocketData()
{
	fd_set set = {};
	int nHighestSockValue = 0;
	int ret;
	P2PSocket* readySocket = NULL;
	UNREFERENCED_VARIABLE(ret); // only referenced in ASSERT
	size_t nSocketCount = m_socketList.size();

	if (nSocketCount == 0)
	{
		// sleep 100ms 
		usleep(100000);
		return NULL;
	}
	// rotation gives another socket priority in the next loop
	m_rotation = (m_rotation + 1) % nSocketCount;
	ASSERT(m_rotation >= 0);

	FD_ZERO(&set);

	for (size_t index = 0; index < nSocketCount; index++)
	{
		ASSERT(m_socketList[index] != NULL);
		int sock = m_socketList[index]->GetSocketHandle();
		ASSERT(sock != -1);
		FD_SET(sock, &set);
		nHighestSockValue = (sock > nHighestSockValue) ? sock : nHighestSockValue;
	}

	// wait indefinitely for a socket
	ret = ::select(nHighestSockValue + 1, &set, NULL, NULL, NULL);

	ASSERT(ret > 0); // This will be a begin assert, and should never happen. But I will want to know if it does

	// now figure out which socket just got data on it
	for (size_t index = 0; index < nSocketCount; index++)
	{
		int indexconverted = (index + m_rotation) % nSocketCount;
		int sock = m_socketList[indexconverted]->GetSocketHandle();

		ASSERT(sock != -1);

		if (FD_ISSET(sock, &set))
		{
			readySocket = m_socketList[indexconverted];
			break;
		}
	}

	ASSERT(readySocket != NULL);

	return readySocket;
}

void P2PSocketThread::Run()
{
	int recvflags = MSG_DONTWAIT;
	int ret;
	P2PSocket* socket;
	char szIPRemote[100] = {};
	char szIPLocal[100] = {};
	CSocketAddress addrRemote;
	CSocketAddress addrLocal;

	Logging::LogMsg(LL_DEBUG, "Starting istener thread");
	printf("Starting istener thread\n");

	while (m_fNeedToExit == false)
	{
		socket = WaitForSocketData();

		if (m_fNeedToExit)
		{
			break;
		}

		ASSERT(socket != NULL);

		if (socket == NULL)
		{
			// just go back to waiting;
			continue;
		}

		// now receive the data
		m_spBufferIn->SetSize(0);

		ret = ::recvfromex(socket->GetSocketHandle(), m_spBufferIn->GetData(), m_spBufferIn->GetAllocatedSize(), recvflags, &addrRemote, &addrLocal);

		// recvfromex no longer sets the port value on the local address
		if (ret >= 0)
		{
			addrLocal.SetPort(socket->GetLocalAddress().GetPort());
		}

		if (Logging::GetLogLevel() >= LL_VERBOSE)
		{
			addrRemote.ToStringBuffer(szIPRemote, 100);
			addrLocal.ToStringBuffer(szIPLocal, 100);
		}
		else
		{
			szIPRemote[0] = '\0';
			szIPLocal[0] = '\0';
		}

		Logging::LogMsg(LL_VERBOSE,  "recvfrom returns %d from %s on local interface %s",
				ret, szIPRemote, szIPLocal);
		
		addrRemote.ToStringBuffer(szIPRemote, 100);
		addrLocal.ToStringBuffer(szIPLocal, 100);
		//printf("recvfrom returns %d from %s on local interface %s\n",
		//		ret, szIPRemote, szIPLocal);

		if (m_fNeedToExit)
		{
			break;
		}

		m_spBufferIn->SetSize(ret);

		// process request
		IMsgHandle* phandle = socket->GetMsgHandle();
		if (phandle != NULL)
		{
			phandle->HandleMsg(m_spBufferIn, addrRemote, addrLocal, (int)socket->GetSocketHandle());
		}
	}

	Logging::LogMsg(LL_DEBUG, "Thread exiting");

	printf("Istener thread exiting\n");
}

HRESULT P2PSocketThread::AddP2PSocket(P2PSocket* socket)
{
	m_socketList.push_back(socket);

	return S_OK;
}

HRESULT P2PSocketThread::RemoveP2PSocket(P2PSocket* socket)
{
	for (p2p_socket_list_t::iterator it = m_socketList.begin(); 
			it != m_socketList.end();
			it++)
	{
		if (*it == socket)
		{
			m_socketList.erase(it);
			break;
		}
	}

	return S_OK;
}
