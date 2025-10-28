#include "Scanner.h"

Scanner::Scanner() {
	if (!WSAManager::InitializeWinsock()) throw;
	localIps = {};
}

void Scanner::ScanPorts(std::string const& host, int minPort, int maxPort, bool fastScan)
{
	std::atomic<int> next_port{ minPort };
	std::atomic<bool> local_stop_scan = false;

	// Функция для рабочего потока
	auto worker = [this, host = host, maxPort = maxPort](std::atomic<int>& next_port_ref) {
		while (!this->stop_scan) {
			int port = next_port_ref++;
			if (port > maxPort) break;

			// Пытаемся подключиться
			if (TryConnectWithTimeout(host, port, 300)) { // 300ms таймаут
				sockaddr_in addr{};
				addr.sin_family = AF_INET;
				inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
				AddOpenedPort(addr, port);
			}
		}
		};

	// Быстрая функция для рабочего потока
	auto fast_worker = [this, host = host, maxPort = maxPort]
	(std::atomic<int>& next_port_ref, std::atomic<bool>& local_stop_scan_ref) {
		while (!this->stop_scan && !local_stop_scan_ref) {
			int port = next_port_ref++;
			if (port > maxPort) break;

			// Пытаемся подключиться
			if (TryConnectWithTimeout(host, port, 100)) {
				sockaddr_in addr{};
				addr.sin_family = AF_INET;
				inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
				AddOpenedPort(addr, port);
				local_stop_scan_ref = true;
			}
		}
		};

	// Создаем ограниченное количество потоков для портов
	const size_t num_port_threads = min(MAX_PORT_THREADS, static_cast<size_t>(maxPort - minPort + 1));

	{
		std::lock_guard<std::mutex> lock(port_threads_mutex);
		if (fastScan)
			for (size_t i = 0; i < num_port_threads; i++) {
				port_threads.emplace_back(fast_worker, std::ref(next_port), std::ref(local_stop_scan));
			}
		else
			for (size_t i = 0; i < num_port_threads; i++) {
				port_threads.emplace_back(worker, std::ref(next_port));
			}
	}

	std::lock_guard<std::mutex> lock(port_threads_mutex);
	{
		// Ждем завершения всех потоков портов
		for (auto& thread : port_threads) {
			if (thread.joinable()) {
				thread.join();
			}
		}
	}
}

void Scanner::StopScan()
{
	stop_scan = true;
	WaitForAllThreads();
}

void Scanner::ScanLocalIps(std::string host, int minPort, int maxPort)
{
	stop_scan = false;
	ip_threads.clear();
	port_threads.clear();
	std::string info{};

	for (int i = 1; i < 256; i++) { // обычно .0 и .255 зарезервированы
		if (stop_scan) break;

		std::string addr = host + "." + std::to_string(i);

		if (i % 8 == 0) {
			PrintScanInfo(info, i == 0);
		}

		// Ждем, если достигли максимума потоков
		if (ip_threads.size() >= MAX_IP_THREADS) {
			// Очищаем завершенные потоки
			for (auto it = ip_threads.begin(); it != ip_threads.end(); ) {
				if (it->joinable()) {
					it->join();
					it = ip_threads.erase(it);
				}
				else {
					++it;
				}
			}
		}

		// Создаем новый поток для сканирования этого IP
		ip_threads.emplace_back([this, addr, minPort, maxPort]() {
			this->ScanPorts(addr, minPort, maxPort, true);
			});
	}

	// Ждем завершения всех потоков
	for (auto& thread : ip_threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
}

std::vector<int> Scanner::GetOpenedPorts(std::string ip)
{
	std::lock_guard<std::mutex> lock(results_mutex);

	auto it = localIps.find(ip);
	if (it != localIps.end()) {
		std::vector<int> temp = std::vector<int>(it->second.begin(), it->second.end());
		return temp;
	}
	else {
		return std::vector<int>();
	}
}

std::set<std::string> Scanner::GetLocalIps()
{
	std::lock_guard<std::mutex> lock(results_mutex);

	std::set<std::string> temp = {};
	for (auto& pair : localIps)
	{
		temp.insert(pair.first);
	}
	return temp;
}

Scanner::~Scanner()
{
	StopScan();
	WaitForAllThreads();
	WSACleanup();
}

void Scanner::TrySocketConnect(SOCKET& connect_socket, sockaddr_in& temp, int port)
{
	int status = connect(connect_socket, (sockaddr*)&temp, sizeof(temp));
	if (status != SOCKET_ERROR) {
		AddOpenedPort(temp, port);
	}

	closesocket(connect_socket);
}

bool Scanner::TryConnectWithTimeout(const std::string& host, int port, int timeout_ms) {
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) return false;

	// Делаем сокет неблокирующим
	u_long mode = 1;
	ioctlsocket(sock, FIONBIO, &mode);

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

	// Пытаемся подключиться
	connect(sock, (sockaddr*)&addr, sizeof(addr));

	// Используем select для таймаута
	fd_set writefds = {};
	FD_ZERO(&writefds);
	FD_SET(sock, &writefds);

	timeval timeout = {};
	timeout.tv_sec = timeout_ms / 1000;
	timeout.tv_usec = (timeout_ms % 1000) * 1000;

	int result = select(0, nullptr, &writefds, nullptr, &timeout);

	bool success = (result > 0) && (FD_ISSET(sock, &writefds));

	closesocket(sock);
	return success;
}

void Scanner::AddOpenedPort(sockaddr_in& addr, int port)
{
	char buff[INET_ADDRSTRLEN];
	InetNtopA(addr.sin_family, &addr.sin_addr, buff, sizeof(buff));
	std::string ip = buff;

	std::lock_guard<std::mutex> lock(results_mutex);

	auto it = localIps.find(ip);
	if (it != localIps.end()) {
		it->second.insert(port);
	}
	else {
		localIps[ip] = { port };
	}
}

void Scanner::PrintScanInfo(std::string& info, bool isFirstPrint)
{
	std::lock_guard<std::mutex> lock(results_mutex);

	info = "Найдено локальных IP-адресов: " + std::to_string(localIps.size());

	std::cout << '\r' << Logger::LogTime() << info;
}

void Scanner::WaitForAllThreads() {
	// Ждем завершения всех IP-потоков
	for (auto& thread : ip_threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
	ip_threads.clear();

	// Ждем завершения всех порт-потоков
	for (auto& thread : port_threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
	port_threads.clear();
}