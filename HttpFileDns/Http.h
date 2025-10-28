#pragma once

#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <string>
#include <map>
#include "WSAManager.h"

class Http
{
public:

	struct ParsedURL {
		std::string host;
		std::string path;
		std::string port;
	};

	enum class HttpMethod { GET_, POST_, PUT_, DELETE_ };

	Http();

	ParsedURL ParseURL(const std::string& url);

	SOCKET CreateConnection(const std::string& host, const std::string& port);

	std::string BuildHTTPRequest(const ParsedURL& url,
		HttpMethod method, const std::string& body = "",
		const std::map<std::string, std::string>& headers = {});

	bool SendHTTPRequest(SOCKET socket, const std::string& request);

	std::string ReceiveHTTPResponse(SOCKET socket);

	void ParseHTTPResponse(const std::string& response);

	void HandleHTTP(const std::string& url, 
		Http::HttpMethod method, const std::string& body = "",
		const std::map<std::string, std::string>& headers = {});

	~Http();
};

