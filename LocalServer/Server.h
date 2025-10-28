#pragma once
#include <winsock2.h>
#include <iostream>
#include <string>
#include <array>
#include <Ws2tcpip.h>
#include <cassert>
#include "Logger.h"
#include "File.h"
#include <windows.h>
#include <shellapi.h>
#include "WSAManager.h"
#pragma comment(lib, "ws2_32.lib")

class Server
{
public:
	
	SOCKET AcceptClient(SOCKET& server_sock);

	void SendResponse(SOCKET client_socket, std::string body = "");

	std::string RecvResponse(SOCKET client_socket);

	std::string GetMyIp4(SOCKET sock);

	std::string GetClientIp4(SOCKET sock);

	void Start();

	~Server();

private:
	SOCKET server_socket;
	std::string bodyTemplate = "<html><body><h1>Привет, сервер работает ;)</h1></body></html>\r\n";
	std::string bodyResponse = bodyTemplate;
};

