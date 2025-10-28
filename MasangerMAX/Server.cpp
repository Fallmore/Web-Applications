#include "Server.h"

SOCKET Server::AcceptClient(SOCKET& server_sock) {
	sockaddr_storage client_addr;
	int client_addr_length = sizeof(client_addr);

	SOCKET client_socket = accept(server_sock, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_length);
	//std::this_thread::sleep_for(5000ms);

	if (!client_socket)
	{
		throw std::system_error(errno, std::system_category(), "accept");
	}
	//assert(sizeof(sockaddr_in) == client_addr_length);

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
	std::string log;
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
			std::string req = std::string(buffer.begin(), std::next(buffer.begin(),
				recv_bytes));
			std::cout << "		------------\n		"
				<< req << std::endl;
			
			API_request r = ParseRequest(req);
			DoRequest(r, client_socket);
			continue;
		}
		else if (0 == recv_bytes) 
		{
			RemoveClient(client_socket);
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
	return true;
}

API_request Server::ParseRequest(const std::string& req)
{
	const char *delimiter = ";";
	char* next_token1 = NULL;
	char* splt = strtok_s(const_cast<char*>(req.c_str()), delimiter, &next_token1);
	API_request api;
	api.action = static_cast<API>(std::stoi(splt));

	splt = strtok_s(nullptr, delimiter, &next_token1);
	while (splt != nullptr)
	{
		api.args.push_back(splt);
		splt = strtok_s(nullptr, delimiter, &next_token1);
	}

	return api;
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
	default:
		break;
	}
}

bool Server::SendResponse(SOCKET& sock, const std::string& request)
{
	// Функция отправки данных "в общем виде".
	size_t req_pos = 0;
	const auto req_length = request.length();
	while (req_pos < req_length)
	{
		if (int bytes_count = send(sock, request.c_str() + req_pos, req_length - req_pos, 0) < 0)
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

std::vector<client_info>::const_iterator Server::GetClientIterator(SOCKET& client_socket)
{
	auto it = std::find(clients.begin(), clients.end(),
		[client_socket](const client_info& info) { return info.client_socket == client_socket; });
	return it;
}

int Server::SendMessagesToClients(API& action, common_chat& chat)
{
	switch (action) {
		case
	}
}

void Server::AddClient(API_request& req, SOCKET& client_socket)
{
	clients.push_back({ client_socket, req.args[0], Logger::LogTime()});
	Logger::Log("Клиент " + GetMyIp4(client_socket) + " зарегистрировался под именем " + req.args[0]);
}

void Server::RemoveClient(SOCKET& client_socket)
{
	auto it = GetClientIterator(client_socket);

	Logger::Log("Клиент " + GetMyIp4(client_socket) + " под именем " + (*it).name + " отключился");
	clients.erase(it);
}

bool Server::ShowClientList(SOCKET& client_socket)
{
	std::string response = "Список клиентов: \n";
	for (auto client : clients)
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
	std::string message = "";
	
	std::string name_group_chat;
	group_chat gchat;

	p2p_chat pchat;
	client_info reciever;
	client_info members[2];

	client_info sender = (*GetClientIterator(client_socket));
	switch (req.action) {
	case send_message_in_common_chat:
		// На случай, если сообщение разделилось сепаратором в виде символа ';'
		for (auto& part : req.args) message += part + ";";
		Chat::send_message(chats.c_chat, message, sender);

		Logger::Log("Клиент " + GetMyIp4(client_socket)
			+ " под именем " + (*GetClientIterator(client_socket)).name + " отправил сообщение в общий чат");
		break;
	case send_message_in_group_chat:
		name_group_chat = req.args[0];
		gchat = *(std::find(chats.group_chats.begin(), chats.group_chats.end(),
			[name_group_chat](const group_chat& chat) { return chat.name == name_group_chat; }));

		// На случай, если сообщение разделилось сепаратором в виде символа ';'
		for (int i = 1; i < req.args.size(); i++) message += req.args[i] + ";";
		Chat::send_message(gchat, message, sender);

		Logger::Log("Клиент " + GetMyIp4(client_socket)
			+ " под именем " + (*GetClientIterator(client_socket)).name + " отправил сообщение в групповой чат");
		break;
	case send_message_in_p2p_chat:
		reciever = { (SOCKET)(std::stoi(req.args[0])), req.args[1] };
		members[0] = sender; members[1] = reciever;
		pchat = *(std::find(chats.p2p_chats.begin(), chats.p2p_chats.end(),
			[members](const p2p_chat& chat) { return chat == members; }));

		// На случай, если сообщение разделилось сепаратором в виде символа ';'
		for (int i = 2; i < req.args.size(); i++) message += req.args[i] + ";";
		Chat::send_message(pchat, message, sender);

		Logger::Log("Клиент " + GetMyIp4(client_socket)
			+ " под именем " + (*GetClientIterator(client_socket)).name + " отправил сообщение в p2p чат");
		break;
	default:
		break;
	}
}

bool Server::SendFileInChat(API_request& req, SOCKET& client_socket)
{
	std::string path = "";

	std::string name_group_chat;
	group_chat gchat;

	p2p_chat pchat;
	client_info reciever;
	client_info members[2];

	client_info sender = (*GetClientIterator(client_socket));
	switch (req.action) {
	case send_file_in_common_chat:
		// На случай, если путь разделился сепаратором в виде символа ';'
		for (auto& part : req.args) path += part + ";";

		Chat::send_file(chats.c_chat, path, sender);
		Logger::Log("Клиент " + GetMyIp4(client_socket)
			+ " под именем " + (*GetClientIterator(client_socket)).name + " отправил файл в общий чат");
		break;
	case send_file_in_group_chat:
		name_group_chat = req.args[0];
		gchat = *(std::find(chats.group_chats.begin(), chats.group_chats.end(),
			[name_group_chat](const group_chat& chat) { return chat.name == name_group_chat; }));
		
		// На случай, если путь разделился сепаратором в виде символа ';'
		for (int i = 1; i < req.args.size(); i++) path += req.args[i] + ";";
		Chat::send_file(gchat, path, sender);

		Logger::Log("Клиент " + GetMyIp4(client_socket)
			+ " под именем " + (*GetClientIterator(client_socket)).name + " отправил файл в групповой чат");
		break;
	case send_file_in_p2p_chat:
		reciever = { (SOCKET)(std::stoi(req.args[0])), req.args[1] };
		members[0] = sender; members[1] = reciever;
		pchat = *(std::find(chats.p2p_chats.begin(), chats.p2p_chats.end(),
			[members](const p2p_chat& chat) { return chat == members; }));
		
		// На случай, если путь разделился сепаратором в виде символа ';'
		for (int i = 2; i < req.args.size(); i++) path += req.args[i] + ";";
		Chat::send_file(pchat, path, sender);

		Logger::Log("Клиент " + GetMyIp4(client_socket)
			+ " под именем " + (*GetClientIterator(client_socket)).name + " отправил файл в p2p чат");
		break;
	default:
		break;
	}
}

bool Server::CreateChat(API_request& req, SOCKET& client_socket)
{
	std::vector<client_info> clnts;

	switch (req.action) {
	case create_group_chat:
		for (size_t i = 1; i < req.args.size(); i+=2)
			clnts.push_back({ (SOCKET)(std::stoi(req.args[i])), req.args[i+1] });
		chats.group_chats.push_back(Chat::create_group_chat(req.args[0], clnts));
		break;
	case create_p2p_chat:
		break;
	default:
		break;
	}
}

bool Server::Start(std::string host, int port) {
	if (!WSAManager::InitializeWinsock()) return false;

	char buff[INET_ADDRSTRLEN];
	sockaddr_in server_addr, client_addr;

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
		FD_ZERO(&rset);
		FD_ZERO(&wset);
	}

	InetNtopA(AF_INET, &server_addr.sin_addr.s_addr, buff, sizeof(buff));
	Logger::Log("Сервер запущен на http://" + std::string(buff) + ":" + std::to_string(port) + "\n");

	int nready;
	// Рабочий цикл сервера.
	while (true) {
		
		FD_SET(server_socket, &rset);

		nready = select(MAXSOCKS, &rset, NULL, NULL, NULL);

		if (FD_ISSET(server_socket, &rset)) {
			int len = sizeof(client_addr);
			int res = accept(server_socket, (sockaddr*)&client_addr, &len);
			/*childpid = fork();
			if ()*/
		}

		SOCKET client_socket = AcceptClient(server_socket);

		if (!RecvRequest(client_socket)) {
			Logger::Log("Клиент " + GetMyIp4(client_socket) + " отключен");
			closesocket(client_socket);
		}
	}

	return true;
}

Server::~Server() {
	closesocket(server_socket);
	WSACleanup();

	Logger::Log("Сервер выключен");
}