#pragma once
#include "api.h"
#include "FileUtils.h"

struct common_chat {
	std::vector<std::string> messages;
	std::vector<std::string> file_paths;

	virtual ~common_chat() = default;
	virtual void on_message_sent(const std::string& message, client_info& sender);
	virtual void on_file_sent(const std::string& file_path, client_info& sender);
};

struct group_chat : common_chat {
	std::string name;
	std::vector<client_info> members;
	void on_message_sent(const std::string& message, client_info& sender) override;
	void on_file_sent(const std::string& message, client_info& sender) override;
};

struct p2p_chat : common_chat {
	client_info member1;
	client_info member2;

	bool operator==(const p2p_chat& ch) const;
};

struct chats {
	common_chat c_chat;
	std::vector<group_chat> group_chats;
	std::vector<p2p_chat> p2p_chats;
};

class Chat
{
public:
	void static send_message(common_chat& chat, std::string& message, client_info& sender);
	void static send_file(common_chat& chat, std::string& file_path, client_info& sender);

	group_chat static create_group_chat(std::string name, std::vector<client_info>& members);
	p2p_chat static create_p2p_chat(client_info& member1, client_info& member2);
};

