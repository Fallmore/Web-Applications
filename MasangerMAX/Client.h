#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <string>
#include "WSAManager.h"

struct client_info {
	SOCKET client_socket;
	std::string name;
	std::string register_time;

	bool operator==(const client_info& client) const;
};

class Client
{
public:

	SOCKET CreateConnection(const std::string& host, const std::string& port);

	bool SendResponse(const std::string& request);

	void Disconnect();

	Client();

	~Client();

private:
	SOCKET server_sock;
};

