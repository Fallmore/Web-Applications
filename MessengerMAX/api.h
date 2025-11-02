#pragma once
#include <vector>
#include <string>
#include "WSAManager.h"
#include "Logger.h"

static const char* separator = "/|\\";
constexpr int MAX_RECV_BUFFER_SIZE = 256;

struct client_info {
	SOCKET client_socket;
	std::string name;
	std::string register_time;

	bool operator==(const client_info& client) const;
};

enum API {
	invalid_connect,
	error,
	register_yourself,
	show_client_list,
	create_group_chat,
	create_p2p_chat,
	send_message_in_common_chat,
	send_message_in_group_chat,
	send_message_in_p2p_chat,
	send_file_in_common_chat,
	send_file_in_group_chat,
	send_file_in_p2p_chat,
	get_file_from_common_chat,
	get_file_from_group_chat,
	get_file_from_p2p_chat
};

struct API_request {
	API action;
	std::vector<std::string> args;
};

class IAPI
{
public:
	static API_request ParseToApi(const std::string& req);
	static std::string ParseToString(const API_request& req);

	virtual bool ShowClientList(SOCKET& client_socket) = 0;

	virtual bool SendMessageInChat(API_request& req, SOCKET& client_socket) = 0;

	virtual bool SendFileInChat(API_request& req, SOCKET& client_socket) = 0;

	virtual bool CreateChat(API_request& req, SOCKET& client_socket) = 0;

	virtual bool GetFile(API_request& req, SOCKET& client_socket) = 0;
};

