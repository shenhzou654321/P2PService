/*************************************************************************
    > File Name: config.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年12月30日 星期三 16时39分45秒
 ************************************************************************/

#include "config.h"
#include <json/json.h>
#include <iostream>
#include <stdio.h>
#include <assert.h>

using namespace std;

Config::Config()
{
	m_deviceSN = "9000000000";
	m_deviceSw = "0.0.0.1";
	m_apiKey = "test1";
	m_privateKey = "000000";
//	m_bcsUri = "api.tbcloud.cc";
	m_bcsUri = "192.168.1.128";
	m_bcsPort = 8080;
	m_heartBeatInterval = 5000;		// 心跳间隔5000ms
	m_productType = "ClientDemo";
	m_reqMaxRetry = 3;
}

bool Config::Load(const char* filepath)
{
	assert(filepath != NULL);
	int length = 0;
	char* buf = NULL;
	FILE *fp = fopen(filepath, "r");
	if (fp == NULL)
	{
		std::cout << "Cann't open config file:" << filepath << endl;
		return false;
	}
	buf = new char[1025];
	length = fread(buf, 1, 1024, fp);
	buf[length] = 0;
	fclose(fp);

	try
	{
		Json::Reader reader;
		Json::Value configValue;
		if (!reader.parse((char*)buf, configValue))
		{
			std::cout << "Parse config file failed:" << filepath << endl;
			std::cout << buf << endl;
			delete buf;
			return false;
		}
		if (!configValue["deviceSN"].isNull())
		{
			m_deviceSN = configValue["deviceSN"].asString();
		}
		if (!configValue["bcsUri"].isNull())
		{
			m_bcsUri = configValue["bcsUri"].asString();
		}
		if (!configValue["bcsPort"].isNull())
		{
			m_bcsPort = configValue["bcsPort"].asInt();
		}
		if (!configValue["heartBeatInterval"].isNull())
		{
			m_heartBeatInterval = configValue["heartBeatInterval"].asInt();
		}
		if (!configValue["apiKey"].isNull())
		{
			m_apiKey = configValue["apiKey"].asString();
		}
		if (!configValue["privateKey"].isNull())
		{
			m_privateKey = configValue["privateKey"].asString();
		}
		delete buf;
		return true;
	}
	catch (...)
	{
		std::cout << "Parse config file failed:" << filepath << endl;
		delete buf;
		return false;
	}
}

Config* Config::Instance()
{
	static Config* obj = NULL;
	if (obj == NULL)
	{
		obj = new Config();
	}
	return obj;
}

