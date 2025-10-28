#include "Server.h"

SOCKET Server::AcceptClient(SOCKET& server_sock) {
	sockaddr_storage client_addr;
	int client_addr_length = sizeof(client_addr);

	SOCKET client_socket = accept(server_sock, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_length);

	if (!client_socket)
	{
		throw std::system_error(errno, std::system_category(), "accept");
	}
	assert(sizeof(sockaddr_in) == client_addr_length);
	std::array<char, INET_ADDRSTRLEN> addr;

	Logger::Log("Клиент принят\n		Адрес клиента: " + std::string(inet_ntop(AF_INET,
		&(reinterpret_cast<const sockaddr_in* const>(&client_addr)->sin_addr), &addr[0], addr.size())) + "\n"
		+ "		Мой адрес: " + GetMyIp4(server_sock));

	return client_socket;
}

std::string Server::GetMyIp4(SOCKET sock)
{
	sockaddr_in addr;
	socklen_t address_len(sizeof(addr));
	// Получить локальный адрес.
	if (int res = getsockname(sock, reinterpret_cast<sockaddr*>(&addr), &address_len) != 0)
	{
		std::cerr << "getsockname: " << gai_strerror(res) << std::endl;
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
	// Получить локальный адрес.
	if (int res = getpeername(sock, reinterpret_cast<sockaddr*>(&addr), &address_len) != 0)
	{
		std::cerr << "getsockname: " << gai_strerror(res) << std::endl;
		return "";
	}

	std::string ip(INET_ADDRSTRLEN, 0);
	inet_ntop(AF_INET, &addr.sin_addr, &ip[0], ip.size());
	return ip;
}

void Server::SendResponse(SOCKET client_socket, std::string body) {
	// Простой HTTP-ответ
	std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Connection: close\r\n"
		"\r\n";

	if (body.size() != 0) {
		if (body.find("file:///") != 0) bodyResponse = body + "\r\n";
		else {
			std::string path = body.substr(8); // убираем "file:///"
			bodyResponse = File::Read(path) + "\r\n";
			ShellExecuteA(0, 0, body.c_str(), 0, 0, SW_SHOW);
			Logger::Log("Открыта ссылка " + body);
		}
	}

	response += bodyResponse;

	// Отправляем весь ответ сразу
	int bytes_sent = send(client_socket, response.c_str(), response.size(), 0);
	Logger::Log("Отправлено " + std::to_string(bytes_sent) + " байт клиенту");
}

std::string Server::RecvResponse(SOCKET client_socket) {
	char client_request[1024];
	int request_bytes = recv(client_socket, client_request, sizeof(client_request) - 1, 0);

	std::string strClient_request = std::string(client_request);
	if (request_bytes > 0) {
		client_request[request_bytes] = '\0';
		Logger::Log("Полученный запрос:\n " + std::string(client_request));

		if (strClient_request.find("PUT") == 0) {
			size_t header_end = strClient_request.find("\r\n\r\n");
			return std::string(client_request).substr(header_end + 4);
		}
	}
	return "";
}

void Server::Start() {
	if (!WSAManager::InitializeWinsock()) return;

	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == INVALID_SOCKET) {
		std::cout << "Создание сокета провалено: " << WSAGetLastError() << std::endl;
		return;
	}

	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(8888);

	if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		std::cout << "Bind провален: " << WSAGetLastError() << std::endl;
		closesocket(server_socket);
		return;
	}

	if (listen(server_socket, 1) == SOCKET_ERROR) {
		std::cout << "Listen провалено: " << WSAGetLastError() << std::endl;
		closesocket(server_socket);
		return;
	}

	Logger::Log("Сервер запущен на http://localhost:8888");
	
	// Рабочий цикл сервера.
	while (true) {
		// Функции передается сокет, на котором сервер выполняет прослушивание.
		// Вызов accept() блокирует выполнение до момента подключения клиента.
		SOCKET client_socket = AcceptClient(server_socket);

		SendResponse(client_socket, RecvResponse(client_socket));

		// Даем время на передачу данных
		Sleep(100);

		closesocket(client_socket);
		Logger::Log("Соединение закрыто");
	}

}

Server::~Server() {
	closesocket(server_socket);
	WSACleanup();

	Logger::Log("Сервер выключен");
}