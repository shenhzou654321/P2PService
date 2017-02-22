
#include "commonincludes.hpp"
#include "stuncore.h"
#include "p2psocket.h"

P2PSocket::P2PSocket() :
m_sock(-1),
m_pMsgHandle(NULL)
{
    
}

P2PSocket::~P2PSocket()
{
    Close();
}

void P2PSocket::Reset()
{
    m_sock = -1;
    m_addrlocal = CSocketAddress(0,0);
    m_addrremote = CSocketAddress(0,0);
    m_pMsgHandle = NULL;
}

void P2PSocket::Close()
{
    if (m_sock != -1)
    {
        close(m_sock);
        m_sock = -1;
    }
    Reset();
}

bool P2PSocket::IsValid()
{
    return (m_sock != -1);
}

HRESULT P2PSocket::Attach(int sock)
{
    if (sock == -1)
    {
        ASSERT(false);
        return E_INVALIDARG;
    }
    
    if (sock != m_sock)
    {
        // close any existing socket
        Close(); // this will also call "Reset"
        m_sock = sock;
    }
    
    UpdateAddresses();
    return S_OK;
}

int P2PSocket::Detach()
{
    int sock = m_sock;
    Reset();
    return sock;
}

int P2PSocket::GetSocketHandle() const
{
    return m_sock;
}

const CSocketAddress& P2PSocket::GetLocalAddress() const
{
    return m_addrlocal;
}

const CSocketAddress& P2PSocket::GetRemoteAddress() const
{
    return m_addrremote;
}

IMsgHandle* P2PSocket::GetMsgHandle()
{
    return m_pMsgHandle;
}

void P2PSocket::SetMsgHandle(IMsgHandle* pMsgHandle)
{
	m_pMsgHandle = pMsgHandle;
}

// About the "packet info option"
// What we are trying to do is enable the socket to be able to provide the "destination address"
// for packets we receive.  However, Linux, BSD, and MacOS all differ in what the
// socket option is. And it differs even differently between IPV4 and IPV6 across these operating systems.
// So we have the "try one or the other" implementation based on what's DEFINED
// On some operating systems, there's only one option defined. Other's have both, but only one works!
// So we have to try them both

HRESULT P2PSocket::EnablePktInfoImpl(int level, int option1, int option2, bool fEnable)
{
    HRESULT hr = S_OK;
    int enable = fEnable?1:0;
    int ret = -1;
    
    
    ChkIfA((option1 == -1) && (option2 == -1), E_FAIL);
    
    if (option1 != -1)
    {
        ret = setsockopt(m_sock, level, option1, &enable, sizeof(enable));
    }
    
    if ((ret < 0) && (option2 != -1))
    {
        enable = fEnable?1:0;
        ret = setsockopt(m_sock, level, option2, &enable, sizeof(enable));
    }
    
    ChkIfA(ret < 0, ERRNOHR);
    
Cleanup:
    return hr;
}

HRESULT P2PSocket::EnablePktInfo_IPV4(bool fEnable)
{
    int level = IPPROTO_IP;
    int option1 = -1;
    int option2 = -1;
    
#ifdef IP_PKTINFO
    option1 = IP_PKTINFO;
#endif
    
#ifdef IP_RECVDSTADDR
    option2 = IP_RECVDSTADDR;
#endif
    
    return EnablePktInfoImpl(level, option1, option2, fEnable);
}

HRESULT P2PSocket::EnablePktInfo_IPV6(bool fEnable)
{
    int level = IPPROTO_IPV6;
    int option1 = -1;
    int option2 = -1;
    
#ifdef IPV6_RECVPKTINFO
    option1 = IPV6_RECVPKTINFO;
#endif
    
#ifdef IPV6_PKTINFO
    option2 = IPV6_PKTINFO;
#endif
    
    return EnablePktInfoImpl(level, option1, option2, fEnable);
}

HRESULT P2PSocket::EnablePktInfoOption(bool fEnable)
{
    int family = m_addrlocal.GetFamily();
    HRESULT hr;
    
    if (family == AF_INET)
    {
        hr = EnablePktInfo_IPV4(fEnable);
    }
    else
    {
        hr = EnablePktInfo_IPV6(fEnable);
    }
    
    return hr;
}

HRESULT P2PSocket::SetV6Only(int sock)
{
    int optname = -1;
    int result = 0;
    HRESULT hr = S_OK;
    int enabled = 1;
    
#ifdef IPV6_BINDV6ONLY
    optname = IPV6_BINDV6ONLY;
#elif IPV6_V6ONLY
    optname = IPV6_V6ONLY;
#else
    return E_NOTIMPL;
#endif
    
    result = setsockopt(sock, IPPROTO_IPV6, optname, (char *)&enabled, sizeof(enabled));
    hr = (result == 0) ? S_OK : ERRNOHR ;
         
    return hr;
}

HRESULT P2PSocket::SetNonBlocking(bool fEnable)
{
    HRESULT hr = S_OK;
    int result;
    int flags;
    
    flags = ::fcntl(m_sock, F_GETFL, 0);
    
    ChkIf(flags == -1, ERRNOHR);

    if (fEnable)
    {    
        flags |= O_NONBLOCK;
    }
    else
    {
        flags &= ~(O_NONBLOCK);
    }
    
    result = fcntl(m_sock , F_SETFL , flags);
    
    ChkIf(result == -1, ERRNOHR);
    
Cleanup:
    return hr;
}


void P2PSocket::UpdateAddresses()
{
    sockaddr_storage addrLocal = {};
    sockaddr_storage addrRemote = {};
    socklen_t len;
    int ret;
    
    ASSERT(m_sock != -1);
    if (m_sock == -1)
    {
        return;
    }
    
    
    len = sizeof(addrLocal);
    ret = ::getsockname(m_sock, (sockaddr*)&addrLocal, &len);
    if (ret != -1)
    {
        m_addrlocal = addrLocal;
    }
    
    len = sizeof(addrRemote);
    ret = ::getpeername(m_sock, (sockaddr*)&addrRemote, &len);
    if (ret != -1)
    {
        m_addrremote = addrRemote;
    }
}



HRESULT P2PSocket::InitCommon(int socktype, const CSocketAddress& addrlocal, IMsgHandle* phandle, bool fSetReuseFlag)
{
    int sock = -1;
    int ret;
    HRESULT hr = S_OK;
    
    ASSERT((socktype == SOCK_DGRAM)||(socktype==SOCK_STREAM));
    
    sock = socket(addrlocal.GetFamily(), socktype, 0);
    ChkIf(sock < 0, ERRNOHR);
    
    if (addrlocal.GetFamily() == AF_INET6)
    {
        // Don't allow IPv6 socket to receive binding request from IPv4 client
        // Because if we don't then an IPv4 client will get an IPv6 mapped address in the binding response
        // I'm pretty sure you have to call this before bind()
        // Intentionally ignoring result
        (void)SetV6Only(sock);
    }
    
    if (fSetReuseFlag)
    {
        int fAllow = 1;
        ret = ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &fAllow, sizeof(fAllow));
        ChkIf(ret == -1, ERRNOHR);
    }
    
    ret = bind(sock, addrlocal.GetSockAddr(), addrlocal.GetSockAddrLength());
    ChkIf(ret == -1, ERRNOHR);
    
    Attach(sock);
    sock = -1;
    
    m_pMsgHandle = phandle; 
    
Cleanup:
    if (sock != -1)
    {
        close(sock);
        sock = -1;
    }
    return hr;
}



HRESULT P2PSocket::UDPInit(const CSocketAddress& local, IMsgHandle* phandle)
{
    return InitCommon(SOCK_DGRAM, local, phandle, false);
}

HRESULT P2PSocket::TCPInit(const CSocketAddress& local, IMsgHandle* phandle, bool fSetReuseFlag)
{
    return InitCommon(SOCK_STREAM, local, phandle, fSetReuseFlag);
}

