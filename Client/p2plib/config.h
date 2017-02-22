#ifndef _P2P_CONFIG_H_
#define _P2P_CONFIG_H_

#define MAX_P2P_MESSAGE_SIZE	1600

#include "p2plib.h"
#include <string>

using std::string;
using namespace p2plib;

extern bool g_p2plibInited;
extern p2p_config_t g_p2plibConfig;
extern string g_sDevId;
extern bool g_isRelayNode;

#define THE_DEVID		g_p2plibConfig.devid
#define S_THE_DEVID		g_sDevId.c_str()

#endif // _P2P_CONFIG_H_


