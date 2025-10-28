#include "Client.h"

SOCKET Client::CreateConnection(const std::string& host, const std::string& port) {
	if (server_sock != INVALID_SOCKET) Disconnect();

	struct addrinfo hints, * result;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Получаем информацию о сервере
	int status = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
	if (status != 0) {
		std::cout << "getaddrinfo провалено: " << status << std::endl;
		return INVALID_SOCKET;
	}

	// Создаем сокет
	SOCKET connect_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (connect_socket == INVALID_SOCKET) {
		std::cout << "Создание сокета провалено: " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		return INVALID_SOCKET;
	}

	// Устанавливаем соединение
	status = connect(connect_socket, result->ai_addr, (int)result->ai_addrlen);
	if (status == SOCKET_ERROR) {
		std::cout << "Соединение провалено: " << WSAGetLastError() << std::endl;
		closesocket(connect_socket);
		freeaddrinfo(result);
		return INVALID_SOCKET;
	}

	freeaddrinfo(result);

	// Установка таймаута на получение данных (5 секунд)
	int timeout = 5000;
	setsockopt(connect_socket, SOL_SOCKET, SO_RCVTIMEO,
		(const char*)&timeout, sizeof(timeout));

	server_sock = connect_socket;
	return connect_socket;
}

bool Client::SendResponse(const std::string& request)
{
	int bytes_sent;
	bytes_sent = send(server_sock, request.c_str(), request.length(), 0);

	if (bytes_sent == SOCKET_ERROR) {
		return false;
	}
	return true;
}

void Client::Disconnect()
{
	shutdown(server_sock, SD_BOTH);
	closesocket(server_sock);
}

Client::Client() {
	WSAManager::InitializeWinsock();
	server_sock = {};
}

Client::~Client() {
	closesocket(server_sock);
	WSACleanup();
}

bool client_info::operator==(const client_info& client) const
{
	if (name == client.name && client_socket == client.client_socket)
		return true;
	else return false;
}
