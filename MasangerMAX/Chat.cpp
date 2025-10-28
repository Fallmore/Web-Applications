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
	return gc;
}

p2p_chat Chat::create_p2p_chat(client_info& member1, client_info& member2)
{
	p2p_chat pc;
	pc.member1 = member1;
	pc.member2 = member2;
	return pc;
}

void common_chat::on_message_sent(const std::string& message, client_info& sender)
{
	std::string info = Logger::LogTime() + sender.name + ": ";
	messages.push_back(info + message);
}

void common_chat::on_file_sent(const std::string& file_path, client_info& sender)
{
	std::string info = Logger::LogTime() + sender.name + " отправил файл: " + file_path;
	messages.push_back(info);

	file_paths.push_back(file_path);
}

//int common_chat::notify_members(API action, std::string& new_message, std::vector<client_info>& members)
//{
//	// Функция отправки данных "в общем виде".
//	size_t req_pos = 0;
//	const auto req_length = new_message.length();
//	int notified = 0;
//
//	for (auto member : members)
//	{
//		while (req_pos < req_length)
//		{
//			if (int bytes_count = send(member.client_socket, new_message.c_str() + req_pos, req_length - req_pos, 0) < 0)
//			{
//			}
//			else
//			{
//				// Сместить указатель на свободное место в буфере.
//				req_pos += bytes_count;
//				notified++;
//			}
//		}
//	}
//	return notified;
//}

void group_chat::on_message_sent(const std::string& message, client_info& sender)
{
	common_chat::on_message_sent(message, sender);

}

void group_chat::on_file_sent(const std::string& message, client_info& sender)
{
	common_chat::on_file_sent(message, sender);
}

bool p2p_chat::operator==(const client_info members[2]) const
{
	return (member1 == members[0] && member2 == members[1]) || (member1 == members[1] && member2 == members[0]);
}
