#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <string>
#include <array>
#include <Ws2tcpip.h>
#include <cassert>
#include "Logger.h"
#include "WSAManager.h"

constexpr auto MAX_RECV_BUFFER_SIZE = 256;

class Server
{
public:

	SOCKET AcceptClient(SOCKET& server_sock);

	bool RecvResponse(SOCKET client_socket);

	std::string GetMyIp4(SOCKET sock);

	std::string GetClientIp4(SOCKET sock);

	bool Start(std::string host, int port);

	bool IsClientDisconnect(WSAEVENT closeEvent, WSANETWORKEVENTS &hDisconnectEvent);

	~Server();

private:
	SOCKET server_socket;
};

