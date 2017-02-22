#ifndef _P2P_SOCKET_THREAD_H_
#define _P2P_SOCKET_THREAD_H_

#include "p2psocket.h"
#include <vector>

using std::vector;

class P2PSocketThread
{
public:
    P2PSocketThread();
    ~P2PSocketThread();

    HRESULT Init();
    HRESULT Start();

    HRESULT SignalForStop(bool fPostMessage);
    HRESULT WaitForStopAndClose();
    HRESULT AddP2PSocket(P2PSocket* socket);
    HRESULT RemoveP2PSocket(P2PSocket* socket);

private:
    void Run();
    
    static void* ThreadFunction(void* pThis);

    P2PSocket* WaitForSocketData();
    HRESULT InitThreadBuffers();
    void UninitThreadBuffers();
    void ClearSocketArray();

private:
    typedef vector<P2PSocket*> p2p_socket_list_t;

    p2p_socket_list_t m_socketList;
    
    bool m_fNeedToExit;
    pthread_t m_pthread;
    bool m_fThreadIsValid;
    
    int m_rotation;

    CRefCountedBuffer m_spBufferIn;
};

#endif // _P2P_SOCKET_THREAD_H_

