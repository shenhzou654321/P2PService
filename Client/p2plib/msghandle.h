#ifndef _MSG_HANDLE_H_
#define _MSG_HANDLE_H_

#include "socketaddress.h"
#include "buffer.h"

class IMsgHandle
{
public:
    virtual void HandleMsg(CRefCountedBuffer &spBufferIn, 
			CSocketAddress &fromAddr,
			CSocketAddress &localAddr,
			int sock) = 0;
};

#endif // _MSG_HANDLE_H_


