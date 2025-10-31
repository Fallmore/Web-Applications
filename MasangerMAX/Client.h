#pragma once
#include "chat.h"
#include <thread>
#include <mutex>
#include <array>
#include <chrono>

class Client
{
public:

	chats GetChats();

	common_chat& GetChat(API_request& request);
	
	common_chat& GetChatUnsafe(API_request& request);

	API_request GetResponse();

	SOCKET CreateConnection(const std::string& host, const std::string& port);

	bool RecvResponse();

	bool SendRequest(API_request& request);
	
	void ManageResponse();

	void StartListening();

	void Listening();

	void Disconnect();

	~Client();

	client_info we;

	std::atomic<bool> is_response_get = false, continue_listening = false;
private:

	SOCKET server_sock = INVALID_SOCKET;
	fd_set rset;

	std::mutex sock_mutex, response_mutex, chats_mutex;
	std::thread listen_t;

	chats chats;
	API_request response;
};

