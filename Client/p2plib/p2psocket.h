#ifndef _P2P_SOCKET_H_
#define _P2P_SOCKET_H_

#include "msghandle.h"

class P2PSocket
{
private:
    int m_sock;
    IMsgHandle* m_pMsgHandle;
    CSocketAddress m_addrlocal;
    CSocketAddress m_addrremote;

    P2PSocket(const P2PSocket&){;}
    void operator=(const P2PSocket&){;}

    HRESULT InitCommon(int socktype, const CSocketAddress& addrlocal, IMsgHandle* phandle, bool fSetReuseFlag);

    void Reset();
    
    HRESULT EnablePktInfoImpl(int level, int option1, int option2, bool fEnalbe);
    HRESULT EnablePktInfo_IPV4(bool fEnable);
    HRESULT EnablePktInfo_IPV6(bool fEnable);

    HRESULT SetV6Only(int sock);

public:
    P2PSocket();
    ~P2PSocket();

    void Close();
    
    
    bool IsValid();
    
    HRESULT Attach(int sock);
    int Detach();
    
    int GetSocketHandle() const;
    const CSocketAddress& GetLocalAddress() const;
    const CSocketAddress& GetRemoteAddress() const;
    
    HRESULT EnablePktInfoOption(bool fEnable);
    HRESULT SetNonBlocking(bool fEnable);
   
    IMsgHandle* GetMsgHandle(); 
	void SetMsgHandle(IMsgHandle* pMsgHandle);
    
    void UpdateAddresses();
    
    HRESULT UDPInit(const CSocketAddress& local, IMsgHandle* phandle);
    HRESULT TCPInit(const CSocketAddress& local, IMsgHandle* phandle, bool fSetReuseFlag);
};

typedef boost::shared_ptr<P2PSocket> CRefCountedP2PSocket;

#endif // _P2P_SOCKET_H_

