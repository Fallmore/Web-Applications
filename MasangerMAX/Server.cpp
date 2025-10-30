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
		Logger::Log("Получено сообщение от " + GetMyIp4(client_socket) + " в размере " + std::to_string(recv_bytes) + " байт...");

		if (recv_bytes > 0)
		{
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
			std::cout << "Получение сообщения провалено: " << WSAGetLastError() << std::endl;
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
	DoRequest(r, client_socket);
	return true;
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
	timeval timeout = {0, 10};
	// Рабочий цикл сервера.
	while (true) {
		SOCKET client_socket;
		FD_ZERO(&rset);
		FD_SET(server_socket, &rset);

		nready = select(MAXSOCKS, &rset, NULL, NULL, &timeout);

		if (FD_ISSET(server_socket, &rset)) {
			client_socket = AcceptClient(server_socket);
			FD_SET(client_socket, &rset);
		}
		nready = select(MAXSOCKS, &rset, NULL, NULL, &timeout);
		if (FD_ISSET(client_socket, &rset)) {
			if (!RecvRequest(client_socket)) {
			}
		}

		//TO DO пересмотреть select и циклы
		for (auto& client : clients) {
			FD_SET(client.client_socket, &rset);
			nready = select(MAXSOCKS, &rset, NULL, NULL, &timeout);
			if (FD_ISSET(client.client_socket, &rset)) {
				if (!RecvRequest(client.client_socket)) {
					RemoveClient(client.client_socket);
				}
			}
		}
	}

	return true;
}

void Server::DoRequest(API_request& req, SOCKET& client_socket)
{
	switch (req.action) {
	case register_yourself:
		AddClient(req, client_socket);
		break;
	case show_client_list:
		ShowClientList(client_socket);
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
		closesocket(client_socket);
		break;
	}
}

bool Server::SendResponse(SOCKET& sock, const std::string& response)
{
	// Функция отправки данных "в общем виде".
	size_t req_pos = 0;
	const auto req_length = response.length();

	while (req_pos < req_length)
	{
		if (int bytes_count = send(sock, response.c_str() + req_pos, req_length - req_pos, 0) < 0)
		{
			RemoveClient(sock);
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
	return it;
}

void Server::AddClient(API_request& req, SOCKET& client_socket)
{
	clients.push_back({ client_socket, req.args[0], Logger::LogTime() });

	std::string response = "";
	response += register_yourself + separator;
	response += client_socket + separator + req.args[0];
	if (SendResponse(client_socket, response))
	{
		Logger::Log("Клиент " + GetMyIp4(client_socket) + " зарегистрировался под именем " + req.args[0]);
	}
}

void Server::RemoveClient(SOCKET& client_socket)
{
	std::string log = "Клиент " + GetMyIp4(client_socket);
	auto it = GetClientIterator(client_socket);

	if (it == clients.end()) {
		Logger::Log(log + " не зарегистрировался и вышел");
		return;
	}

	Logger::Log(log + " под именем " + (*it).name + " отключился");
	closesocket(client_socket);
	clients.erase(it);
}

bool Server::ShowClientList(SOCKET& client_socket)
{
	std::string response = std::to_string(show_client_list) + separator + "Список клиентов: \n";
	for (auto& client : clients)
	{
		response += client.name + ", Время регистрации: " + client.register_time + "\n";
	}
	if (SendResponse(client_socket, response))
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
	common_chat target_chat;
	std::string log = "Клиент " + GetMyIp4(client_socket)
		+ " под именем " + (*GetClientIterator(client_socket)).name;

	std::string name_group_chat = "";
	group_chat gchat;
	std::vector<group_chat>::iterator gchit;

	p2p_chat pchat;
	client_info sender = (*GetClientIterator(client_socket));
	client_info reciever;
	p2p_chat temp_pchat;
	std::vector<p2p_chat>::iterator pchit;

	response += req.action;
	switch (req.action) {
	case send_message_in_common_chat:
		// На случай, если сообщение разделилось сепаратором
		for (auto& part : req.args) message += part + separator;
		Chat::send_message(chats.c_chat, message, sender);

		recievers = std::vector<client_info>(clients);
		target_chat = chats.c_chat;

		Logger::Log(log + " отправил сообщение в общий чат");
		break;
	case send_message_in_group_chat:
		name_group_chat = req.args[0];
		response += separator + name_group_chat;

		gchit = std::find_if(chats.group_chats.begin(), chats.group_chats.end(),
			[name_group_chat](const group_chat& chat) { return chat.name == name_group_chat; });
		if (gchit == chats.group_chats.end()) {
			Logger::Log(log + " попытался написать в несуществующий групповой чат");
			return false;
		}

		// На случай, если сообщение разделилось сепаратором
		for (int i = 1; i < req.args.size(); i++) message += req.args[i] + separator;
		Chat::send_message((*gchit), message, sender);

		recievers = std::vector<client_info>((*gchit).members);
		target_chat = (*gchit);

		Logger::Log(log + " отправил сообщение в групповой чат");
		break;
	case send_message_in_p2p_chat:
		reciever = { (SOCKET)(std::stoi(req.args[0])), req.args[1] };
		temp_pchat.member1 = reciever; temp_pchat.member2 = sender;

		pchit = std::find_if(chats.p2p_chats.begin(), chats.p2p_chats.end(),
			[temp_pchat](const p2p_chat& chat) { return chat == temp_pchat; });
		if (pchit == chats.p2p_chats.end()) {
			Logger::Log(log + " попытался написать в несуществующий p2p чат");
			return false;
		}

		// На случай, если сообщение разделилось сепаратором
		for (int i = 2; i < req.args.size(); i++) message += req.args[i] + separator;
		Chat::send_message(*pchit, message, sender);

		recievers = { (*pchit).member1, (*pchit).member2 };
		target_chat = (*pchit);

		Logger::Log(log + " отправил сообщение в p2p чат");
		break;
	default:
		break;
	}

	if (req.action != send_message_in_p2p_chat) {
		response += separator + target_chat.messages.back();
		for (auto& rreciever : recievers) {
			SendResponse(rreciever.client_socket, response);
		}
	}
	else {
		response = req.action + separator
			+ std::to_string(recievers[1].client_socket) + separator
			+ recievers[1].name + separator
			+ target_chat.messages.back();
		SendResponse(recievers[0].client_socket, response);

		response = req.action + separator
			+ std::to_string(recievers[0].client_socket) + separator
			+ recievers[0].name + separator
			+ target_chat.messages.back();
		SendResponse(recievers[1].client_socket, response);
	}
	return true;
}

bool Server::SendFileInChat(API_request& req, SOCKET& client_socket)
{
	std::string path = "server temp/", response = "";;
	std::vector<client_info> recievers;
	common_chat target_chat;
	std::string log = "Клиент " + GetMyIp4(client_socket)
		+ " под именем " + (*GetClientIterator(client_socket)).name;

	std::string name_group_chat;
	group_chat gchat;
	std::vector<group_chat>::iterator gchit;

	p2p_chat pchat;
	client_info sender = (*GetClientIterator(client_socket));
	client_info reciever;
	p2p_chat temp_pchat;
	std::vector<p2p_chat>::iterator pchit;

	response += req.action;
	switch (req.action) {
	case send_file_in_common_chat:
		path += req.args[0];
		if (!FileUtils::WriteFile(path)) {
			Logger::Log(log + " попытался отправить файл в общий чат, но произошла ошибка");
			SendResponse(client_socket, std::to_string(error) + separator + "Не удалось отправить файл");
			return false;
		}

		Chat::send_file(chats.c_chat, path, sender);

		recievers = std::vector<client_info>(clients);
		target_chat = chats.c_chat;

		Logger::Log(log + " отправил файл в общий чат");
		break;
	case send_file_in_group_chat:
		name_group_chat = req.args[0];
		response += separator + name_group_chat;
		gchit = std::find_if(chats.group_chats.begin(), chats.group_chats.end(),
			[name_group_chat](const group_chat& chat) { return chat.name == name_group_chat; });
		if (gchit == chats.group_chats.end()) {
			Logger::Log(log + " попытался отправить файл в несуществующий групповой чат");
			return false;
		}

		path += req.args[1];
		if (!FileUtils::WriteFile(path)) {
			Logger::Log(log + " попытался отправить файл в групповой чат, но произошла ошибка");
			SendResponse(client_socket, std::to_string(error) + separator + "Не удалось отправить файл");
			return false;
		}

		Chat::send_file(*gchit, path, sender);

		recievers = std::vector<client_info>((*gchit).members);
		target_chat = *gchit;

		Logger::Log(log + " отправил файл в групповой чат");
		break;
	case send_file_in_p2p_chat:
		reciever = { (SOCKET)(std::stoi(req.args[0])), req.args[1] };
		temp_pchat.member1 = reciever; temp_pchat.member2 = sender;

		pchit = std::find_if(chats.p2p_chats.begin(), chats.p2p_chats.end(),
			[temp_pchat](const p2p_chat& chat) { return chat == temp_pchat; });
		if (pchit == chats.p2p_chats.end()) {
			Logger::Log(log + " попытался отправить файл в несуществующий p2p чат");
			return false;
		}

		path += req.args[0];
		if (!FileUtils::WriteFile(path)) {
			Logger::Log(log + " попытался отправить файл в p2p чат, но произошла ошибка");
			SendResponse(client_socket, std::to_string(error) + separator + "Не удалось отправить файл");
			return false;
		}

		Chat::send_file(*pchit, path, sender);

		recievers = { (*pchit).member1, (*pchit).member2 };
		target_chat = *pchit;

		Logger::Log(log + " отправил файл в p2p чат");
		break;
	default:
		break;
	}

	if (req.action != send_file_in_p2p_chat) {
		response += separator + target_chat.messages.back()
			+ separator + target_chat.file_paths.back();
		for (auto& rreciever : recievers) {
			SendResponse(rreciever.client_socket, response);
		}
	}
	else {
		response = req.action + separator
			+ std::to_string(recievers[1].client_socket) + separator
			+ recievers[1].name + separator
			+ target_chat.messages.back() + separator
			+ target_chat.file_paths.back();
		SendResponse(recievers[0].client_socket, response);

		response = req.action + separator
			+ std::to_string(recievers[0].client_socket) + separator
			+ recievers[0].name + separator
			+ target_chat.messages.back() + separator
			+ target_chat.file_paths.back();
		SendResponse(recievers[1].client_socket, response);
	}
	return true;
}

bool Server::CreateChat(API_request& req, SOCKET& client_socket)
{
	std::string log = "Клиент " + GetMyIp4(client_socket)
		+ " под именем " + (*GetClientIterator(client_socket)).name;
	std::string response = "";

	std::vector<client_info> clnts;
	group_chat gc;
	p2p_chat pc;
	common_chat target_chat;

	response += req.action;
	switch (req.action) {
	case create_group_chat:
		response += separator + req.args[0];
		for (size_t i = 1; i < req.args.size(); i += 2) {

			clnts.push_back({ (SOCKET)(std::stoi(req.args[i])), req.args[i + 1] });
			response += separator + req.args[i] + separator + req.args[i + 1];
		}

		gc = Chat::create_group_chat(req.args[0], clnts);
		target_chat = gc;

		chats.group_chats.push_back(gc);

		break;
	case create_p2p_chat:
		for (size_t i = 0; i < req.args.size(); i += 2) {
			clnts.push_back({ (SOCKET)(std::stoi(req.args[i])), req.args[i + 1] });
			response += separator + req.args[i] + separator + req.args[i + 1];
		}

		pc = Chat::create_p2p_chat(clnts[0], clnts[1]);
		target_chat = pc;

		chats.p2p_chats.push_back(pc);
		break;
	default:
		break;
	}

	for (auto& reciever : clnts) {
		SendResponse(reciever.client_socket, response);
	}
	return true;
}

bool Server::GetFile(API_request& req, SOCKET& client_socket)
{
	std::string log = "Клиент " + GetMyIp4(client_socket)
		+ " под именем " + (*GetClientIterator(client_socket)).name;
	std::string content = FileUtils::GetFile(req.args[0]);

	if (SendResponse(client_socket, req.action + separator + content)) {
		Logger::Log(log + " скачал файл " + req.args[0]);
		return true;
	}

	return false;
}

Server::~Server() {
	closesocket(server_socket);
	for (auto& client : clients) {
		closesocket(client.client_socket);
	}
	WSACleanup();

	Logger::Log("Сервер выключен");
}