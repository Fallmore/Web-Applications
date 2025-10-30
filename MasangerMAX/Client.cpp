#include "Client.h"

SOCKET Client::CreateConnection(const std::string& host, const std::string& port) {
	if (!WSAManager::InitializeWinsock()) return INVALID_SOCKET;
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

bool Client::RecvResponse()
{
	std::array<char, MAX_RECV_BUFFER_SIZE> buffer;
	std::string temp = "";

	std::lock_guard<std::mutex> lock(sock_mutex);
	while (true)
	{
		// Прочитать данные. Если данных нет, будет возвращен -1, а errno
		// установлена в 0, то есть отсутствие ошибки.
		const auto recv_bytes = recv(server_sock, buffer.data(), buffer.size() - 1, 0);

		if (recv_bytes > 0)
		{
			// Создать из буфера строку и вывести на консоль.
			buffer[recv_bytes] = '\0';
			temp += std::string(buffer.begin(), std::next(buffer.begin(), recv_bytes));

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

	std::lock_guard<std::mutex> lock2(response_mutex);
	response = IAPI::ParseToApi(temp);
	is_response_get = true;
	ManageResponse();

	return true;
}

bool Client::SendRequest(API_request& request)
{
	// Функция отправки данных "в общем виде".
	size_t req_pos = 0;
	std::string req = IAPI::ParseToString(request);
	const auto req_length = req.length();

	std::lock_guard<std::mutex> lock(sock_mutex);
	while (req_pos < req_length)
	{
		if (int bytes_count = send(server_sock, req.c_str() + req_pos, req_length - req_pos, 0) < 0)
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

void Client::ManageResponse()
{
	std::vector<client_info> clnts;

	group_chat gc;
	std::string name_group_chat = "", path = "client temp/";
	std::vector<group_chat>::iterator gchit;

	p2p_chat pc, pc_temp;
	std::vector<p2p_chat>::iterator pchit;
	client_info member;

	std::lock_guard<std::mutex> lock2(response_mutex);
	if (is_response_get) {
		switch (response.action) {
		case error:
			std::cout << response.args[0];
			break;
		case register_yourself:
			std::cout << "Вы успешно зарегистрировались";
			we.client_socket = (SOCKET)std::stoi(response.args[0]);
			we.name = response.args[1];
			break;
		case show_client_list:
			for (auto& mes : response.args) std::cout << mes;
			break;
#pragma region Create
		case create_group_chat:
			for (size_t i = 1; i < response.args.size(); i += 2) {
				clnts.push_back({ (SOCKET)(std::stoi(response.args[i])), response.args[i + 1] });
			}

			gc = Chat::create_group_chat(response.args[0], clnts);
			chats.group_chats.push_back(gc);

			break;
		case create_p2p_chat:
			for (size_t i = 0; i < response.args.size(); i += 2) {
				clnts.push_back({ (SOCKET)(std::stoi(response.args[i])), response.args[i + 1] });
			}

			pc = Chat::create_p2p_chat(clnts[0], clnts[1]);
			chats.p2p_chats.push_back(pc);
			break;
#pragma endregion
#pragma region SendMessage
		case send_message_in_common_chat:
			for (auto& mes : response.args) chats.c_chat.messages.push_back(mes);
			break;
		case send_message_in_group_chat:
			name_group_chat = response.args[0];
			gchit = std::find_if(chats.group_chats.begin(), chats.group_chats.end(),
				[name_group_chat](const group_chat& chat) { return chat.name == name_group_chat; });
			if (gchit == chats.group_chats.end()) {
				std::cout << "Упс, чат больше не существует или написали не тебе";
			}

			// На случай, если сообщение разделилось сепаратором
			for (int i = 1; i < response.args.size(); i++) gchit->messages.push_back(response.args[i]);

			break;
		case send_message_in_p2p_chat:
			pc_temp.member1 = we;
			pc_temp.member2 = { (SOCKET)(std::stoi(response.args[0])), response.args[1] };

			pchit = std::find_if(chats.p2p_chats.begin(), chats.p2p_chats.end(),
				[pc_temp](const p2p_chat& chat) { return chat == pc_temp; });
			if (pchit == chats.p2p_chats.end()) {
				std::cout << "Упс, чат больше не существует или написали не тебе";
			}

			// На случай, если сообщение разделилось сепаратором
			for (int i = 2; i < response.args.size(); i++) pchit->messages.push_back(response.args[i]);

			break;
#pragma endregion
#pragma region SendFile
		case send_file_in_common_chat:
			for (auto& mes : response.args) chats.c_chat.messages.push_back(mes);
			break;
		case send_file_in_group_chat:
			name_group_chat = response.args[0];
			gchit = std::find_if(chats.group_chats.begin(), chats.group_chats.end(),
				[name_group_chat](const group_chat& chat) { return chat.name == name_group_chat; });
			if (gchit == chats.group_chats.end()) {
				std::cout << "Упс, чат больше не существует или написали не тебе";
			}

			gchit->messages.push_back(response.args[1]);
			gchit->file_paths.push_back(response.args[2]);
			break;
		case send_file_in_p2p_chat:
			pc_temp.member1 = we;
			pc_temp.member2 = { (SOCKET)(std::stoi(response.args[0])), response.args[1] };

			pchit = std::find_if(chats.p2p_chats.begin(), chats.p2p_chats.end(),
				[pc_temp](const p2p_chat& chat) { return chat == pc_temp; });
			if (pchit == chats.p2p_chats.end()) {
				std::cout << "Упс, чат больше не существует или написали не тебе";
			}

			gchit->messages.push_back(response.args[2]);
			gchit->file_paths.push_back(response.args[3]);
			break;
#pragma endregion
#pragma region GetFile
		case get_file_from_common_chat:
		case get_file_from_group_chat:
		case get_file_from_p2p_chat:
			path += response.args[0];
			if (!FileUtils::WriteFile(path)) {
				std::cout << "Не удалось записать файл";
			}
			else {
				std::cout << "Файл успешно скачан и находится на пути " + path;
			}
			break;
#pragma endregion
		default:
			Disconnect();
			break;
		}
		Sleep(100);
		is_response_get = false;
	}
}

void Client::StartListening()
{
	listen_t = std::thread(&Client::Listening, this);
	if (listen_t.joinable())
		listen_t.join();
}

void Client::Listening()
{
	timeval timeout;

	using namespace std::chrono_literals;
	int nready;
	while (continue_listening) {
		FD_ZERO(&rset);
		FD_SET(server_sock, &rset);

		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		nready = select(1, &rset, NULL, NULL, &timeout);

		if (nready == 0) {
			continue;
		}
		else if (nready > 0) {
			if (FD_ISSET(server_sock, &rset))
				RecvResponse();
		}
		else break;
	}
}

void Client::Disconnect()
{
	continue_listening = false;
	if (listen_t.joinable())
		listen_t.join();
	shutdown(server_sock, SD_BOTH);
	closesocket(server_sock);
}

Client::~Client() {
	Disconnect();
	WSACleanup();
}
