#pragma once
#include <vector>
#include <string>
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>

enum API {
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
	//TO DO v
	get_messages_from_common_chat,
	get_messages_from_group_chat,
	get_messages_from_p2p_chat,
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
	virtual void AddClient(API_request& req, SOCKET& client_socket);

	virtual void RemoveClient(SOCKET& client_socket);

	virtual bool ShowClientList(SOCKET& client_socket);

	virtual bool SendMessageInChat(API_request& req, SOCKET& client_socket);

	virtual bool SendFileInChat(API_request& req, SOCKET& client_socket);

	virtual bool CreateChat(API_request& req, SOCKET& client_socket);

	virtual bool GetMessages(API_request& req, SOCKET& client_socket);

	virtual bool GetFile(API_request& req, SOCKET& client_socket);
};

