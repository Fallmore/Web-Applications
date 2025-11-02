#include "Chat.h"

void Chat::send_message(common_chat& chat, std::string& message, client_info& sender)
{
	chat.on_message_sent(message, sender);
}

void Chat::send_file(common_chat& chat, std::string& file_path, client_info& sender)
{
	chat.on_file_sent(file_path, sender);
}

group_chat Chat::create_group_chat(std::string name, std::vector<client_info>& members)
{
	group_chat gc;
	gc.members = members;
	gc.name = name;
	gc.messages.push_back(Logger::LogTime() + "Групповой чат создан, приятного общения :)");
	return gc;
}

p2p_chat Chat::create_p2p_chat(client_info& member1, client_info& member2)
{
	p2p_chat pc;
	pc.member1 = member1;
	pc.member2 = member2;
	pc.messages.push_back(Logger::LogTime() + "Чат создан, поздоровайтесь ;)");
	return pc;
}

void common_chat::on_message_sent(const std::string& message, client_info& sender)
{
	std::string info = Logger::LogTime() + sender.name + ": ";
	messages.push_back(info + message);
}

void common_chat::on_file_sent(const std::string& file_path, client_info& sender)
{
	std::string file_name = FileUtils::GetFileName(file_path);
	std::string info = Logger::LogTime() + sender.name + " отправил файл " + file_name;
	messages.push_back(info);

	file_paths.push_back(file_name);
}

void group_chat::on_message_sent(const std::string& message, client_info& sender)
{
	common_chat::on_message_sent(message, sender);

}

void group_chat::on_file_sent(const std::string& message, client_info& sender)
{
	common_chat::on_file_sent(message, sender);
}

bool p2p_chat::operator==(const p2p_chat& ch) const
{
	return (member1 == ch.member1 && member2 == ch.member2) || (member1 == ch.member2 && member2 == ch.member1);
}
