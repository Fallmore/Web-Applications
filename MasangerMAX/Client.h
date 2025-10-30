#pragma once
#include "chat.h"
#include <thread>
#include <mutex>
#include <array>
#include <chrono>

class Client
{
public:

	SOCKET CreateConnection(const std::string& host, const std::string& port);

	bool RecvResponse();

	bool SendRequest(API_request& request);
	
	void ManageResponse();

	void StartListening();

	void Listening();

	void Disconnect();

	~Client();

	chats chats;
	client_info we;

	API_request response;
	std::atomic<bool> is_response_get = false, continue_listening = false;
private:
	SOCKET server_sock;
	fd_set rset;

	std::mutex sock_mutex, response_mutex;
	std::thread listen_t;
};

