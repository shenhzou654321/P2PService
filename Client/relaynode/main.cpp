/*************************************************************************
    > File Name: main.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年12月30日 星期三 15时35分36秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "devengine.h"
#include "config.h"

int main(int argc, char** argv)
{
	string configfile = "relaynode.json";
	for (int i=0; i<argc; i++)
	{
		if (strcmp(argv[i], "-c") == 0)
		{
			if (i+1 < argc)
			{
				configfile = argv[i+1];
			}
		}
	}
	if (!Config::Instance()->Load(configfile.c_str()))
	{
		return -1;
	}

	DevEngine* pdevEngine = DevEngine::Instance();

	if (!pdevEngine->Init())
	{
		return -1;
	}

	if (!pdevEngine->Start(false))
	{
		return -1;
	}

	pdevEngine->Stop();
	DevEngine::Destory();

	return 0;
}



