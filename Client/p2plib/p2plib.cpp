/*************************************************************************
    > File Name: p2plib.cpp
    > Author: ma6174
    > Mail: ma6174@163.com 
    > Created Time: 2015年11月20日 星期五 14时08分08秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "p2plib.h"
#include "p2plibimpl.h"
#include <string>
#include <sstream>

using namespace p2plib;
using std::string;
using std::stringstream;

bool g_p2plibInited = false;
p2p_config_t g_p2plibConfig;
IP2PAgent* g_p2pAgent = NULL;
uint32_t g_lastError = P2P_OK;
string g_sDevId;
bool g_isRelayNode = false;

int P2P_Init(const p2plib::p2p_config_t *config, bool isRelayNode)
{
	if (config == NULL)
	{
		return P2P_SYSTEM_ERROR;
	}
	g_p2plibInited = true;
	g_p2plibConfig = *config;
	g_isRelayNode = isRelayNode;

	stringstream ss;
	ss << (int)config->devid;
	g_sDevId = ss.str();
	return P2P_OK;
}

void P2P_Fini()
{
	P2P_DestoryP2PAgent(g_p2pAgent);
	g_p2plibInited = false;
	return;
}

p2plib::IP2PAgent* P2P_CreateP2PAgent()
{
	if (!g_p2plibInited)
	{
		P2P_SetLastError(P2P_LIB_NOT_INIT);
		return NULL;
	}
	if (g_p2pAgent != NULL)
	{
		P2P_SetLastError(P2P_AGENT_CREATED);
		return NULL;
	}
	g_p2pAgent = (IP2PAgent*)new P2PAgentImpl(g_p2plibConfig);
	return g_p2pAgent;
}

void P2P_DestoryP2PAgent(p2plib::IP2PAgent* pP2PAgent)
{
	ASSERT(g_p2pAgent == pP2PAgent);
	if (g_p2pAgent != pP2PAgent)
	{
		return;
	}
	if (g_p2pAgent != NULL)
	{
		delete g_p2pAgent;
		g_p2pAgent = NULL;
	}
	return;
}

uint32_t P2P_LastError()
{
	return g_lastError;
}

void P2P_SetLastError(uint32_t err)
{
	g_lastError = err;
}
