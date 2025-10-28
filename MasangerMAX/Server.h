#pragma once
#include <iostream>
#include <array>
#include <Ws2tcpip.h>
#include <cassert>
#include "Logger.h"
#include <chrono>
#include <thread>
#include "WSAManager.h"
#include "Chat.h"
#include "Client.h"
#include "api.h"

#define ioctl ioctlsocket
#define MAXSOCKS 10

using std::chrono_literals::operator""ms;
constexpr auto MAX_RECV_BUFFER_SIZE = 256;

class Server : IAPI
{
public:

	SOCKET AcceptClient(SOCKET& server_sock);

	std::string GetMyIp4(SOCKET sock);

	std::string GetClientIp4(SOCKET sock);

	bool RecvRequest(SOCKET client_socket);

	API_request ParseRequest(const std::string& req);

	void DoRequest(API_request& req, SOCKET& client_socket);

	bool SendResponse(SOCKET& sock, const std::string& request);

	void AddClient(API_request& req, SOCKET& client_socket);

	void RemoveClient(SOCKET& client_socket);

	bool ShowClientList(SOCKET& client_socket);

	bool SendMessageInChat(API_request& req, SOCKET& client_socket);

	bool SendFileInChat(API_request& req, SOCKET& client_socket);

	bool CreateChat(API_request& req, SOCKET& client_socket);

	bool GetMessages(API_request& req, SOCKET& client_socket);
	
	bool GetFile(API_request& req, SOCKET& client_socket);

	bool Start(std::string host, int port);

	~Server();

private:
	std::vector<client_info>::const_iterator GetClientIterator(SOCKET& client_socket);
	int SendMessagesToClients(API& action, common_chat& chat);

	SOCKET server_socket;
	fd_set rset, wset;

	std::vector<client_info> clients;
	chats chats;
};

