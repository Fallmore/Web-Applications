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

	Logger::Log("Клиент принят\n		Адрес клиента: " + GetMyIp4(client_socket));

	return client_socket;
}

bool Server::RecvResponse(SOCKET client_socket) {
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
			std::cout << "		------------\n		"
				<<
				std::string(buffer.begin(), std::next(buffer.begin(),
					recv_bytes))
				<<
				std::endl;
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
	return true;
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

	//Привязка и включение прослушивания
	{
		if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
			std::cout << "Bind провален: " << WSAGetLastError() << std::endl;
			closesocket(server_socket);
			return false;
		}

		if (listen(server_socket, 1) == SOCKET_ERROR) {
			std::cout << "Listen провалено: " << WSAGetLastError() << std::endl;
			closesocket(server_socket);
			return false;
		}
	}

	InetNtopA(AF_INET, &server_addr.sin_addr.s_addr, buff, sizeof(buff));
	Logger::Log("Сервер запущен на http://" + std::string(buff) + ":" + std::to_string(port) + "\n");

	// Рабочий цикл сервера.
	while (true) {

		SOCKET client_socket = AcceptClient(server_socket);

		if (!RecvResponse(client_socket)) {
			Logger::Log("Клиент " + GetMyIp4(client_socket) + " отключен");
			closesocket(client_socket);
		}
			
		/*if (!IsClientDisconnect(closeEvent, hDisconnectEvent)) {
		}
		else {
			closesocket(client_socket);
			Logger::Log("Клиент " + GetMyIp4(client_socket) + " отключен");
			return true;
		}*/
	}

	return true;
}

bool Server::IsClientDisconnect(WSAEVENT closeEvent, WSANETWORKEVENTS& hDisconnectEvent) {
	WSAWaitForMultipleEvents(1, &closeEvent, false, 1, false);
	WSAEnumNetworkEvents(server_socket, closeEvent, &hDisconnectEvent);
	if ((hDisconnectEvent.lNetworkEvents & FD_CLOSE) &&	//Нужное ли нам событие ?
		//если нет ошибок, то обрабатываем
		(hDisconnectEvent.iErrorCode[FD_CLOSE_BIT] == 0)) {
		return true;
	}
	return false;
}

Server::~Server() {
	closesocket(server_socket);
	WSACleanup();

	Logger::Log("Сервер выключен");
}