#include "Http.h"

Http::Http() {
	WSAManager::InitializeWinsock();
}

Http::ParsedURL Http::ParseURL(const std::string& url) {
	ParsedURL result;
	result.port = "80"; // порт по умолчанию для HTTP

	// Принудительно меняем https на http
	std::string modified_url = url;
	if (modified_url.find("https://") == 0) {
		modified_url.replace(0, 5, "http"); // https -> http
		std::cout << "Внимание: Исправлено HTTPS на HTTP" << std::endl;
	}

	// Обработка localhost
	if (modified_url.find("http://localhost") == 0) {
		std::string without_protocol = modified_url.substr(7); // убираем "http://"

		size_t slash_pos = without_protocol.find('/');
		if (slash_pos != std::string::npos) {
			std::string host_part = without_protocol.substr(0, slash_pos);
			result.path = without_protocol.substr(slash_pos);

			// Проверяем порт в localhost
			size_t colon_pos = host_part.find(':');
			if (colon_pos != std::string::npos) {
				result.host = host_part.substr(0, colon_pos); // "localhost"
				result.port = host_part.substr(colon_pos + 1);
			}
			else {
				result.host = host_part;
			}
		}
		else {
			result.host = "localhost";
			result.path = "/";
		}
	}
	// Простой парсинг
	else if (modified_url.find("http://") == 0) {
		std::string without_protocol = modified_url.substr(7); // убираем "http://"

		// Ищем путь
		size_t slash_pos = without_protocol.find('/');
		if (slash_pos != std::string::npos) {
			result.host = without_protocol.substr(0, slash_pos);
			result.path = without_protocol.substr(slash_pos);
		}
		else {
			result.host = without_protocol;
			result.path = "/";
		}

		// Проверяем есть ли порт в host
		size_t colon_pos = result.host.find(':');
		if (colon_pos != std::string::npos) {
			result.port = result.host.substr(colon_pos + 1);
			result.host = result.host.substr(0, colon_pos);
		}
	}
	return result;
}

SOCKET Http::CreateConnection(const std::string& host, const std::string& port) {
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
	return connect_socket;
}


std::string Http::BuildHTTPRequest(const ParsedURL& url,
	Http::HttpMethod method, const std::string& body,
	const std::map<std::string, std::string>& headers) {

	std::string method_str;
	switch (method) {
	case HttpMethod::GET_: method_str = "GET"; break;
	case HttpMethod::POST_: method_str = "POST"; break;
	case HttpMethod::PUT_: method_str = "PUT"; break;
	case HttpMethod::DELETE_: method_str = "DELETE"; break;
	}

	std::string request =
		method_str + " " + url.path + " HTTP/1.1\r\n"
		"Host: " + url.host + "\r\n"
		"User-Agent: MyHTTPClient/1.0\r\n"
		"Connection: close\r\n";

	// Добавляем Content-Length для методов с телом
	if ((method == HttpMethod::POST_ || method == HttpMethod::PUT_) && !body.empty()) {
		request += "Content-Length: " + std::to_string(body.length()) + "\r\n";
	}

	// Добавляем пользовательские заголовки
	for (const auto& x : headers) {
		request += x.first + ": " + x.second + "\r\n";
	}

	request += "\r\n"; // конец заголовков

	// Добавляем тело только для соответствующих методов
	if ((method == HttpMethod::POST_ || method == HttpMethod::PUT_) && !body.empty()) {
		request += body;
	}

	return request;
}

bool Http::SendHTTPRequest(SOCKET socket, const std::string& request) {
	int bytes_sent = send(socket, request.c_str(), request.length(), 0);
	if (bytes_sent == SOCKET_ERROR) {
		std::cout << "Отправка провалена: " << WSAGetLastError() << std::endl;
		return false;
	}
	std::cout << "Отправлено " << bytes_sent << " байтов" << std::endl;
	return true;
}

std::string Http::ReceiveHTTPResponse(SOCKET socket) {
	std::string response;
	char buffer[4096];
	int bytes_received;

	// Получаем данные пока они есть
	do {
		bytes_received = recv(socket, buffer, sizeof(buffer) - 1, 0);
		if (bytes_received > 0) {
			buffer[bytes_received] = '\0'; // добавляем нуль-терминатор
			response += buffer;
		}
		else if (bytes_received == 0) {
			std::cout << "Соединение закрыто" << std::endl;
		}
		else {
			std::cout << "Recv провалено: " << WSAGetLastError() << std::endl;
			break;
		}
	} while (bytes_received > 0);

	return response;
}

void Http::ParseHTTPResponse(const std::string& response) {
	// Ищем конец заголовков
	size_t header_end = response.find("\r\n\r\n");
	if (header_end == std::string::npos) {
		std::cout << "Неудачный HTTP ответ" << std::endl;
		return;
	}

	std::string headers = response.substr(0, header_end);
	std::string body = response.substr(header_end + 4);

	// Парсим статусную строку
	size_t first_newline = headers.find('\n');
	if (first_newline != std::string::npos) {
		std::string status_line = headers.substr(0, first_newline);
		std::cout << "Статус: " << status_line << std::endl;
	}

	std::cout << "\n=== Заголовки ===" << std::endl;
	std::cout << headers << std::endl;

	std::cout << "\n=== Тело ===" << std::endl;
	std::cout << body << std::endl;

	std::cout << "\nРазмер всего ответа: " << response.length() << " байт" << std::endl;
	std::cout << "Размер тела: " << body.length() << " байт" << std::endl;
}

void Http::HandleHTTP(const std::string& url, 
	Http::HttpMethod method, const std::string& body,
	const std::map<std::string, std::string>& headers) {

	// Парсим URL
	ParsedURL parsed = ParseURL(url);
	std::cout << "Соединение к: " << parsed.host << ":" << parsed.port << std::endl;
	std::cout << "Путь: " << parsed.path << std::endl;

	// Создаем соединение
	SOCKET client_socket = CreateConnection(parsed.host, parsed.port);
	if (client_socket == INVALID_SOCKET) {
		WSACleanup();
		return;
	}

	// Установка таймаута на получение данных (5 секунд)
	int timeout = 5000;
	setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO,
		(const char*)&timeout, sizeof(timeout));

	// Формируем и отправляем запрос
	std::string request = BuildHTTPRequest(parsed, method, body, headers);
	std::cout << "=== HTTP Запрос ===" << std::endl;
	std::cout << request << std::endl;

	if (!SendHTTPRequest(client_socket, request)) {
		closesocket(client_socket);
		WSACleanup();
		return;
	}

	// Получаем ответ
	std::string response = ReceiveHTTPResponse(client_socket);
	ParseHTTPResponse(response);

	// Завершаем работу
	closesocket(client_socket);
}

Http::~Http()
{
	WSACleanup();
}

