#pragma once
#include <array>
#include <cassert>
#include "chat.h"

#define ioctl ioctlsocket
#define MAXSOCKS 10

class Server : public IAPI
{
public:

	SOCKET AcceptClient(SOCKET& server_sock);

	std::string GetMyIp4(SOCKET sock);

	std::string GetClientIp4(SOCKET sock);

	bool RecvRequest(SOCKET client_socket);

	bool Start(std::string host, int port);

	bool DoRequest(API_request& req, SOCKET& client_socket);

	bool SendResponse(SOCKET& sock, const std::string& response);

	bool AddClient(API_request& req, SOCKET& client_socket);

	std::vector<client_info>::iterator RemoveClient(SOCKET& client_socket);

	bool ShowClientList(SOCKET& client_socket) override;

	bool SendMessageInChat(API_request& req, SOCKET& client_socket) override;

	bool SendFileInChat(API_request& req, SOCKET& client_socket) override;

	bool CreateChat(API_request& req, SOCKET& client_socket) override;

	bool GetFile(API_request& req, SOCKET& client_socket) override;

	~Server();

private:
	std::vector<client_info>::iterator GetClientIterator(SOCKET& client_socket);
	client_info& GetClient(std::string name);
	common_chat& GetChatUnsafe(API_request& req, client_info& sender);

	SOCKET server_socket;
	fd_set rset, wset;

	std::vector<client_info> clients;
	chats chats;
	std::string dst_files = "D:\\New data\\repos\\Web Applications (difficult)\\x64\\Debug\\server temp\\";
};

