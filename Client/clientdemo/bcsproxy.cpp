/*************************************************************************
    > File Name: BcsProxy.cpp
    > Author: PYW
    > Mail: pengyouwei@comtom.cn 
    > Created Time: 2015年12月30日 星期三 19时34分11秒
 ************************************************************************/

#include "commonincludes.hpp"
#include "socketrole.h"
#include "bcsproxy.h"
#include "config.h"
#include "resolvehostname.h"
#include "stunsocket.h"
#include <sstream>
#include <iostream>
#include <exception>
#include <boost/uuid/sha1.hpp>
#include <json/json.h>

using namespace std;

const char RelayNodeType[] = "1";

BcsProxy::BcsProxy()
{

}

BcsProxy::~BcsProxy()
{

}

int BcsProxy::Register(bool requestNewNs)
{
	Config* pconf = Config::Instance();

	if (pconf == NULL)
	{
		return -1;
	}
	m_registerParams["apiKey"] = pconf->ApiKey();
	m_registerParams["devSn"] = pconf->DeviceSN();
	m_registerParams["devType"] = RelayNodeType;
	m_registerParams["devSw"] = pconf->DeviceSw();
	m_registerParams["productType"] = pconf->ProductType();
	m_registerParams["regType"] = requestNewNs ? "1" : "0";

	return DoRegister();
}

int BcsProxy::DoRegister()
{
	int ret = 0;
	try
	{
		string svrUri = Config::Instance()->BCSUri();
		string path = "/dev/register";
		string value;
		CSocketAddress addrServer;
		char szIP[100];
		bool readMsgEnd = false;
		std::stringstream responseStream;
		char buffer[1024];
		HRESULT hr = S_OK;
		
		hr = ::ResolveHostName(svrUri.c_str(), AF_INET, false, &addrServer);
		if (FAILED(hr))
		{
			std::cout << "Resolve host name failed:" << svrUri << endl;
			return -1;
		}
		addrServer.SetPort(Config::Instance()->BCSPort());
		addrServer.ToStringBuffer(szIP, ARRAYSIZE(szIP));
		std::cout << "Resolve " << svrUri << " to " << szIP << endl;

		addrServer.SetPort(Config::Instance()->BCSPort());

		CStunSocket socket;
		CSocketAddress addrLocal = CSocketAddress(0, 0);
		hr = socket.TCPInit(addrLocal, RolePP, true);
		if (FAILED(hr))
		{
			std::cout << "Unable to create local socket for TCP connection (hr == " << (int)hr << endl;
			return -1;
		}

		// connect to server
		int sock = -1;
		sock = socket.GetSocketHandle();
	
		ret = ::connect(sock, addrServer.GetSockAddr(), addrServer.GetSockAddrLength());
		if (ret == -1)
		{
			std::cout << "Can't connect to BCS server" << endl;
			return -1;
		}

		std::ostringstream requestStream;
		requestStream << "GET " << path << "?";
		for (param_map_t::iterator it = m_registerParams.begin();
				it != m_registerParams.end();
				it++)
		{
			requestStream << it->first << "=" << it->second << "&";
		}
		requestStream << "apiSign=" << GetApiSign() << " HTTP/1.0\r\n"
			<< "Host: " << svrUri << ":" << Config::Instance()->BCSPort() << "\r\n"
			<< "Accept: */*\r\n"
			<< "Connection: close\r\n\r\n";

		std::cout << "Send:" << endl
			<< requestStream.str();

		int bytesSent = 0;
		int bytesToSend = (int)requestStream.str().length();
		while (bytesSent < bytesToSend)
		{
			ret = ::send(sock, requestStream.str().c_str() + bytesSent, bytesToSend - bytesSent, 0);
			if (ret < 0)
			{
				std::cout << "Send failed" << endl;
				return -1;
			}
			std::cout << "Send " << ret << "bytes: "<< requestStream.str().c_str() + bytesSent << endl;
			bytesSent += ret;
		}

		// read 
		while (!readMsgEnd)
		{
			ret = ::recv(sock, buffer, sizeof(buffer) - 1, 0);
			if (ret == 0)
			{
				break;
			}
			if (ret < 0)
			{
				std::cout << "Recv failed from BCS" << endl;
				return -1;
			}
			buffer[ret + 1] = 0;
			responseStream << buffer;
		}

		socket.Close();

		std::cout << "Recv:"  << endl
			<< responseStream.str() << endl;

		string httpVersion;
		responseStream >> httpVersion;
		unsigned int statusCode;
		responseStream >> statusCode;

		if (httpVersion.substr(0, 5) != "HTTP/")
		{
			std::cout << "Invalid response\n";
			return -1;
		}
		if (statusCode != 200)
		{
			std::cout << "Response returned with status code" << statusCode << endl;
			return -1;
		}
		string statusMessage;
		std::getline(responseStream, statusMessage);
		// std::cout << "status message:" << statusMessage << endl;

		string header;
		//std::cout << "print header:" << endl;
		while (std::getline(responseStream, header) && header != "\r")
		{
		//	std::cout << header << endl;
		}
		//std::cout << endl;

		string::size_type pos = responseStream.str().find("{\"data");
		if (pos == string::npos)
		{
			std::cout << "Can't find body head: {\"data\"\n";
			return -1;
		}
		string body(responseStream.str().substr(pos));
		pos = body.rfind("}");
		if (pos != string::npos)
		{
			body = body.substr(0, pos + 1);
		}
		//std::cout << "body:" << body << endl;
		
		return ParseBody(body);
	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << endl;
		return -1;
	}
	catch (...)
	{
		std::cout << "Exception" << endl;
		return -1;
	}
}

int BcsProxy::ParseBody(string& body)
{
	Json::Reader reader;
	Json::Value bodyValue;

	if (!reader.parse(body.c_str(), bodyValue)
			|| bodyValue["data"].isNull())
	{
		std::cout << "Parse message body failed: " << body << endl;
		return -1;
	}
	
	const Json::Value& dataValue = bodyValue["data"];

	if (dataValue["devNo"].isNull()
			|| dataValue["nodeServer"].isNull()
			|| dataValue["nodeServerPort"].isNull())
	{
		std::cout << "Parse message data failed" << endl;
		return -1;
	}
	m_devNo = dataValue["devNo"].asInt();
	m_secretKey = dataValue["secretKey"].asString();
	m_nsAddr = dataValue["nodeServer"].asString();
	m_nsPort = dataValue["nodeServerPort"].asInt();
	
	std::cout << "DevNo:" << m_devNo << endl
		<< "SecretKey:" << m_secretKey << endl
		<< "NsAddr:" << m_nsAddr << endl
		<< "NsPort" << m_nsPort << endl;
	return 0;
}

string BcsProxy::GetApiSign()
{
	std::ostringstream paramSs;
	for (param_map_t::iterator iter = m_registerParams.begin();
			iter != m_registerParams.end(); 
			iter++)
	{
		paramSs << iter->first << iter->second;
	}
	paramSs << Config::Instance()->PrivateKey();

	// SHA1 
	boost::uuids::detail::sha1 sha;
	sha.process_bytes(paramSs.str().c_str(), paramSs.str().length());
	unsigned int digest[5];
	sha.get_digest(digest);

	char szSign[64] = {0};
	sprintf(szSign, "%08X%08X%08X%08X%08X", digest[0], digest[1], digest[2], digest[3], digest[4]);

	std::cout << "API sign:"<< szSign << endl;

	return string(szSign);
}

