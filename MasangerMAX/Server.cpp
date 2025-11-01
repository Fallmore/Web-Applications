#include "Server.h"
#include "Client.h"

SOCKET Server::AcceptClient(SOCKET& server_sock) {
	sockaddr_storage client_addr;
	int client_addr_length = sizeof(client_addr);

	SOCKET client_socket = accept(server_sock, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_length);

	if (!client_socket)
	{
		throw std::system_error(errno, std::system_category(), "accept");
	}
	assert(sizeof(sockaddr_in) == client_addr_length);

	Logger::Log("Клиент принят\n		Адрес клиента: " + GetMyIp4(client_socket));

	//Всех задержит
	do
	{
		FD_SET(client_socket, &rset);
		int nready = select(1, &rset, NULL, NULL, NULL);
	} while (!FD_ISSET(client_socket, &rset));

	return client_socket;
}

std::string Server::GetMyIp4(SOCKET sock)
{
	sockaddr_in addr;
	socklen_t address_len(sizeof(addr));
	// Получить локальный адрес.
	if (int res = getsockname(sock, reinterpret_cast<sockaddr*>(&addr), &address_len) != 0)
	{
		std::cerr << "getsockname: " << WSAGetLastError() << std::endl;
		return "";
	}

	std::string ip(INET_ADDRSTRLEN, 0);
	inet_ntop(AF_INET, &addr.sin_addr, &ip[0], ip.size());
	return ip;
}

std::string Server::GetClientIp4(SOCKET sock)
{
	sockaddr_in addr;
	socklen_t address_len(sizeof(addr));
	// Получить адрес удаленного абонента.
	if (int res = getpeername(sock, reinterpret_cast<sockaddr*>(&addr), &address_len) != 0)
	{
		std::cerr << "getsockname: " << WSAGetLastError() << std::endl;
		return "";
	}

	std::string ip(INET_ADDRSTRLEN, 0);
	inet_ntop(AF_INET, &addr.sin_addr, &ip[0], ip.size());
	return ip;
}

bool Server::RecvRequest(SOCKET client_socket) {
	std::array<char, MAX_RECV_BUFFER_SIZE> buffer;
	std::string log, req = "";
	while (true)
	{
		// Прочитать данные. Если данных нет, будет возвращен -1, а errno
		// установлена в 0, то есть отсутствие ошибки.

		const auto recv_bytes = recv(client_socket, buffer.data(), buffer.size() - 1, 0);

		if (recv_bytes > 0)
		{
			Logger::Log("Получено сообщение от " + GetMyIp4(client_socket) + " в размере " + std::to_string(recv_bytes) + " байт...");
			// Создать из буфера строку и вывести на консоль.
			buffer[recv_bytes] = '\0';
			req += std::string(buffer.begin(), std::next(buffer.begin(), recv_bytes));
			std::cout << "		------------\n		"
				<< req << std::endl;
			continue;
		}
		else if (0 == recv_bytes)
		{
			return false;
		}
		else if (-1 == recv_bytes)
		{
			// Для Windows корректнее будет проверять WSAGetLastError()
			// вместо errno.
			if (EINTR == errno) continue;
			if (0 == errno) break;
			// -1 тут не ошибка. Если данных нет, errno будет содержать
			// код EAGAIN или Resource temporarily unavailable.
			// Но здесь это нормально.
			if (EAGAIN == errno) break;
			return false;
		}
		break;
	}

	API_request r = ParseToApi(req);
	return DoRequest(r, client_socket);
}

bool Server::Start(std::string host, int port) {
	if (!WSAManager::InitializeWinsock()) return false;

	char buff[INET_ADDRSTRLEN];
	sockaddr_in server_addr;

	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == INVALID_SOCKET) {
		std::cout << "Создание сокета провалено: " << WSAGetLastError() << std::endl;
		return false;
	}

	server_addr.sin_family = AF_INET;
	inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr);
	server_addr.sin_port = htons(port);

	//Настройка сокета для TCP
	{
		int flag = 1;
		// Перевести сокет в неблокирующий режим.
		// Закомментированный вариант не работает для Windows.
		// Вариант с ioctl()/ioctlsocket() – кросс-платформенный.
		if (int res = ioctl(server_socket, FIONBIO, reinterpret_cast<u_long*>(&flag)) < 0)
		{
			std::cerr << gai_strerror(res) << std::endl;
			return false;
		}
		// Выключить алгоритм Нейгла.
		if (int res = setsockopt(server_socket, IPPROTO_TCP, TCP_NODELAY,
			reinterpret_cast<const char*>(&flag), sizeof(flag)) < 0)
		{
			std::cerr << gai_strerror(res) << std::endl;
			return false;
		}
	}

	//Привязка и включение прослушивания
	{
		if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
			std::cout << "Bind провален: " << WSAGetLastError() << std::endl;
			closesocket(server_socket);
			return false;
		}

		if (listen(server_socket, MAXSOCKS) == SOCKET_ERROR) {
			std::cout << "Listen провалено: " << WSAGetLastError() << std::endl;
			closesocket(server_socket);
			return false;
		}
		FD_ZERO(&wset);
	}

	InetNtopA(AF_INET, &server_addr.sin_addr.s_addr, buff, sizeof(buff));
	Logger::Log("Сервер запущен на http://" + std::string(buff) + ":" + std::to_string(port) + "\n");

	int nready;
	timeval timeout = { 0, 10 };
	// Рабочий цикл сервера.
	while (true) {
		SOCKET client_socket;
		FD_ZERO(&rset);
		FD_SET(server_socket, &rset);

		for (auto& client : clients) {
			FD_SET(client.client_socket, &rset);
		}

		nready = select(MAXSOCKS + 1, &rset, NULL, NULL, &timeout);

		if (nready > 0) {
			// Проверяем новый ли клиент
			if (FD_ISSET(server_socket, &rset)) {
				client_socket = AcceptClient(server_socket);
				if (!RecvRequest(client_socket)) {
					Logger::Log("Клиент " + GetMyIp4(client_socket) + " не смог зарегистрироваться");
				}
				nready--;
			}

			// Обрабатываем клиентов с данными
			for (auto it = clients.begin(); nready > 0 && it != clients.end(); ) {
				if (FD_ISSET(it->client_socket, &rset)) {
					if (!RecvRequest(it->client_socket)) {
						it = RemoveClient(it->client_socket);
					}
					else {
						++it;
					}
					nready--;
				}
				else {
					++it;
				}
			}
		}
	}

	return true;
}

bool Server::DoRequest(API_request& req, SOCKET& client_socket)
{
	switch (req.action) {
	case register_yourself:
		return AddClient(req, client_socket);
		break;
	case show_client_list:
		return ShowClientList(client_socket);
		break;
	case create_group_chat:
	case create_p2p_chat:
		CreateChat(req, client_socket);
		break;
	case send_message_in_common_chat:
	case send_message_in_group_chat:
	case send_message_in_p2p_chat:
		SendMessageInChat(req, client_socket);
		break;
	case send_file_in_common_chat:
	case send_file_in_group_chat:
	case send_file_in_p2p_chat:
		SendFileInChat(req, client_socket);
		break;
	case get_file_from_common_chat:
	case get_file_from_group_chat:
	case get_file_from_p2p_chat:
		GetFile(req, client_socket);
		break;
	default:
		return false;
		break;
	}
	return true;
}

bool Server::SendResponse(SOCKET& sock, const std::string& response)
{
	// Функция отправки данных "в общем виде".
	size_t req_pos = 0;
	const auto req_length = response.length();
	std::string req_send;
	while (req_pos < req_length)
	{
		req_send = response.substr(req_pos, min(req_pos + MAX_RECV_BUFFER_SIZE, req_length));
		int bytes_count = send(sock, req_send.c_str(), sizeof(req_send), 0);
		if (bytes_count < 0)
		{
			return false;
		}
		else
		{
			// Сместить указатель на свободное место в буфере.
			req_pos += bytes_count;
		}
	}
	return true;
}

std::vector<client_info>::iterator Server::GetClientIterator(SOCKET& client_socket)
{
	auto it = std::find_if(clients.begin(), clients.end(),
		[client_socket](const client_info& info) { return info.client_socket == client_socket; });
	if (it == clients.end()) throw std::invalid_argument("Не существует такого клиента по сокету");

	return it;
}

client_info& Server::GetClient(std::string name)
{
	auto it = std::find_if(clients.begin(), clients.end(),
		[name](const client_info& info) { return info.name == name; });
	if (it == clients.end()) throw std::invalid_argument("Не существует такого клиента по имени");
	return *it;
}

bool Server::AddClient(API_request& req, SOCKET& client_socket)
{
	clients.push_back({ client_socket, req.args[0], Logger::LogTime() });

	std::string response = "";
	response += std::to_string(register_yourself) + separator;
	response += std::to_string(client_socket) + separator + req.args[0];
	if (SendResponse(client_socket, response + separator))
	{
		Logger::Log("Клиент " + GetMyIp4(client_socket) + " зарегистрировался под именем " + req.args[0]);
		return true;
	}
	else return false;
}

std::vector<client_info>::iterator Server::RemoveClient(SOCKET& client_socket)
{
	std::string log = "Клиент " + GetMyIp4(client_socket);
	auto it = GetClientIterator(client_socket);

	if (it == clients.end()) {
		Logger::Log(log + " не зарегистрировался и вышел");
		return it;
	}

	Logger::Log(log + " под именем " + (*it).name + " отключился");
	closesocket(client_socket);
	return clients.erase(it);
}

bool Server::ShowClientList(SOCKET& client_socket)
{
	std::string response = std::to_string(show_client_list) + separator;
	for (auto& client : clients)
	{
		response += client.name + ", Время регистрации: " + client.register_time + "\n";
	}
	if (SendResponse(client_socket, response + separator))
	{
		Logger::Log("Клиент " + GetMyIp4(client_socket)
			+ " под именем " + (*GetClientIterator(client_socket)).name + " получил список клиентов");
		return true;
	}
	else return false;
}

bool Server::SendMessageInChat(API_request& req, SOCKET& client_socket)
{
	std::string message = "", response = "";
	std::vector<client_info> recievers;
	client_info& sender = (*GetClientIterator(client_socket));
	std::string log = "Клиент " + GetMyIp4(client_socket)
		+ " под именем " + sender.name;

	try
	{
		common_chat& chat = GetChatUnsafe(req, sender);
		response += std::to_string(req.action);
		switch (req.action) {
		case send_message_in_common_chat:
			// На случай, если сообщение разделилось сепаратором
			for (auto& part : req.args) message += part;
			Chat::send_message(chat, message, sender);

			recievers = std::vector<client_info>(clients);
			Logger::Log(log + " отправил сообщение в общий чат");
			break;
		case send_message_in_group_chat:
			response += separator + req.args[0];

			// На случай, если сообщение разделилось сепаратором
			for (int i = 1; i < req.args.size(); i++) message += req.args[i];
			Chat::send_message(chat, message, sender);

			recievers = std::vector<client_info>((*dynamic_cast<group_chat*>(&chat)).members);
			Logger::Log(log + " отправил сообщение в групповой чат");
			break;
		case send_message_in_p2p_chat:
			// На случай, если сообщение разделилось сепаратором
			for (int i = 2; i < req.args.size(); i++) message += req.args[i];
			Chat::send_message(chat, message, sender);

			recievers = { (*dynamic_cast<p2p_chat*>(&chat)).member1, (*dynamic_cast<p2p_chat*>(&chat)).member2 };
			Logger::Log(log + " отправил сообщение в p2p чат");
			break;
		default:
			break;
		}

		if (req.action != send_message_in_p2p_chat) {
			response += separator + chat.messages.back();
			for (auto& rreciever : recievers) {
				if (!SendResponse(rreciever.client_socket, response + separator)) {
					RemoveClient(rreciever.client_socket);
				}
			}
		}
		else {
			response = std::to_string(req.action) + separator
				+ std::to_string(recievers[1].client_socket) + separator
				+ recievers[1].name + separator
				+ chat.messages.back();
			if (!SendResponse(recievers[0].client_socket, response + separator)) {
				RemoveClient(recievers[0].client_socket);
			}

			response = std::to_string(req.action) + separator
				+ std::to_string(recievers[0].client_socket) + separator
				+ recievers[0].name + separator
				+ chat.messages.back();
			if (!SendResponse(recievers[1].client_socket, response + separator)) {
				RemoveClient(recievers[1].client_socket);
			}
		}
		return true;
	}
	catch (const std::invalid_argument&)
	{
		Logger::Log(log + " попытался написать в несуществующий чат");
		response = std::to_string(error) + separator + "Такого чата больше не существует(";
		if (!SendResponse(client_socket, response + separator)) {
			RemoveClient(client_socket);
		}
		return false;
	}
}

bool Server::SendFileInChat(API_request& req, SOCKET& client_socket)
{
	std::string path, response = "";
	std::vector<client_info> recievers;
	client_info sender = (*GetClientIterator(client_socket));
	std::string log = "Клиент " + GetMyIp4(client_socket)
		+ " под именем " + sender.name;

	try
	{
		common_chat& chat = GetChatUnsafe(req, sender);
		response += std::to_string(req.action);
		switch (req.action) {
		case send_file_in_common_chat:
			path += req.args[0];
			if (!FileUtils::WriteFile(path, dst_files)) {
				Logger::Log(log + " попытался отправить файл в общий чат, но произошла ошибка");
				SendResponse(client_socket, std::to_string(error) + separator + "Не удалось отправить файл" + separator);
				return false;
			}
			Chat::send_file(chat, path, sender);

			recievers = std::vector<client_info>(clients);
			Logger::Log(log + " отправил файл в общий чат");
			break;
		case send_file_in_group_chat:
			response += separator + req.args[0];
			path += req.args[1];
			if (!FileUtils::WriteFile(path, dst_files)) {
				Logger::Log(log + " попытался отправить файл в групповой чат, но произошла ошибка");
				SendResponse(client_socket, std::to_string(error) + separator + "Не удалось отправить файл" + separator);
				return false;
			}
			Chat::send_file(chat, path, sender);

			recievers = std::vector<client_info>((*dynamic_cast<group_chat*>(&chat)).members);
			Logger::Log(log + " отправил файл в групповой чат");
			break;
		case send_file_in_p2p_chat:
			path += req.args[2];
			if (!FileUtils::WriteFile(path, dst_files)) {
				Logger::Log(log + " попытался отправить файл в p2p чат, но произошла ошибка");
				SendResponse(client_socket, std::to_string(error) + separator + "Не удалось отправить файл" + separator);
				return false;
			}
			Chat::send_file(chat, path, sender);

			recievers = { (*dynamic_cast<p2p_chat*>(&chat)).member1, (*dynamic_cast<p2p_chat*>(&chat)).member2 };
			Logger::Log(log + " отправил файл в p2p чат");
			break;
		default:
			break;
		}

		if (req.action != send_file_in_p2p_chat) {
			response += separator + chat.messages.back()
				+ separator + chat.file_paths.back();
			for (auto& rreciever : recievers) {
				if (!SendResponse(rreciever.client_socket, response + separator)) {
					RemoveClient(rreciever.client_socket);
				}
			}
		}
		else {
			response = std::to_string(req.action) + separator
				+ std::to_string(recievers[1].client_socket) + separator
				+ recievers[1].name + separator
				+ chat.messages.back() + separator
				+ chat.file_paths.back();
			if (!SendResponse(recievers[0].client_socket, response + separator)) {
				RemoveClient(recievers[0].client_socket);
			}

			response = std::to_string(req.action) + separator
				+ std::to_string(recievers[0].client_socket) + separator
				+ recievers[0].name + separator
				+ chat.messages.back() + separator
				+ chat.file_paths.back();
			if (!SendResponse(recievers[1].client_socket, response + separator)) {
				RemoveClient(recievers[1].client_socket);
			}
		}
		return true;
	}
	catch (const std::invalid_argument&)
	{
		Logger::Log(log + " попытался отправить файл в несуществующий чат");
		response = std::to_string(error) + separator + "Такого чата больше не существует(";
		if (!SendResponse(client_socket, response + separator)) {
			RemoveClient(client_socket);
		}
		return false;
	}
}

bool Server::CreateChat(API_request& req, SOCKET& client_socket)
{
	std::string log = "Клиент " + GetMyIp4(client_socket)
		+ " под именем " + (*GetClientIterator(client_socket)).name;
	std::string response = "";

	std::vector<client_info> clnts;
	client_info clnt;
	group_chat gc;
	p2p_chat pc;
	common_chat target_chat;

	response += std::to_string(req.action);
	int i = 1;
	switch (req.action) {
	case create_group_chat:
		response += separator + req.args[0];
		try
		{
			for (i = 1; i < req.args.size(); i++) {

				clnt = GetClient(req.args[i]);
				clnts.push_back(clnt);
				response += separator + std::to_string(clnt.client_socket) + separator + clnt.name;
			}
		}
		catch (const std::invalid_argument&)
		{
			Logger::Log(log + " хотел создать групповой чат с " + req.args[i] + " но он вышел");
			response = std::to_string(error) + separator + "Пользователь " + req.args[i] + " вышел(";
			if (!SendResponse(client_socket, response + separator)) {
				RemoveClient(client_socket);
			}
			return false;
		}

		clnt = *GetClientIterator(client_socket);
		response += separator + std::to_string(clnt.client_socket) + separator + clnt.name;
		clnts.push_back(clnt);
		gc = Chat::create_group_chat(req.args[0], clnts);
		target_chat = gc;

		chats.group_chats.push_back(gc);
		Logger::Log(log + " создал групповой чат " + gc.name);
		break;
	case create_p2p_chat:
		try
		{
			clnt = GetClient(req.args[0]);
		}
		catch (const std::invalid_argument&)
		{
			Logger::Log(log + " хотел создать личный чат с " + req.args[0] + " но он вышел");
			response = std::to_string(error) + separator + "Пользователь " + req.args[0] + " вышел(";
			if (!SendResponse(client_socket, response + separator)) {
				RemoveClient(client_socket);
			}
			return false;
		}

		clnts.push_back(clnt);
		response += separator + std::to_string(clnt.client_socket) + separator + clnt.name;

		clnt = *GetClientIterator(client_socket);
		response += separator + std::to_string(clnt.client_socket) + separator + clnt.name;
		clnts.push_back(*GetClientIterator(client_socket));

		pc = Chat::create_p2p_chat(clnts[0], clnts[1]);
		target_chat = pc;

		chats.p2p_chats.push_back(pc);
		Logger::Log(log + " создал p2p чат с " + clnt.name);
		break;
	default:
		break;
	}

	for (auto& reciever : clnts) {
		if (!SendResponse(reciever.client_socket, response + separator)) {
			RemoveClient(reciever.client_socket);
		}
	}
	return true;
}

bool Server::GetFile(API_request& req, SOCKET& client_socket)
{
	std::string log = "Клиент " + GetMyIp4(client_socket)
		+ " под именем " + (*GetClientIterator(client_socket)).name;
	std::string content = FileUtils::GetFile(dst_files + req.args[0]);

	if (SendResponse(client_socket, std::to_string(req.action) + separator + req.args[0] + separator + content + separator)) {
		Logger::Log(log + " скачал файл " + req.args[0]);
		return true;
	}

	return false;
}

common_chat& Server::GetChatUnsafe(API_request& req, client_info& sender)
{
	std::string name_group_chat = "";
	std::vector<group_chat>::iterator gchit;

	p2p_chat pc, pc_temp;
	std::vector<p2p_chat>::iterator pchit;
	client_info member;

	switch (req.action) {
	case send_file_in_common_chat:
	case send_message_in_common_chat:
		return chats.c_chat;
		break;
	case send_file_in_group_chat:
	case send_message_in_group_chat:
		name_group_chat = req.args[0];
		gchit = std::find_if(chats.group_chats.begin(), chats.group_chats.end(),
			[name_group_chat](const group_chat& chat) { return chat.name == name_group_chat; });
		if (gchit == chats.group_chats.end()) {
			throw std::runtime_error("Упс, чат не найден: " + name_group_chat);
		}

		return *gchit;
		break;
	case send_file_in_p2p_chat:
	case send_message_in_p2p_chat:
		pc_temp.member1 = sender;
		pc_temp.member2 = GetClient(req.args[1]);

		pchit = std::find_if(chats.p2p_chats.begin(), chats.p2p_chats.end(),
			[pc_temp](const p2p_chat& chat) { return chat == pc_temp; });
		if (pchit == chats.p2p_chats.end()) {
			throw std::runtime_error("Упс, чат не найден: " + pc_temp.member1.name + " и " + pc_temp.member2.name);
		}
		return *pchit;
		break;
	default:
		throw std::runtime_error("Упс, неизвестный чат");
		break;
	}
}

Server::~Server() {
	closesocket(server_socket);
	for (auto& client : clients) {
		closesocket(client.client_socket);
	}
	WSACleanup();

	Logger::Log("Сервер выключен");
}