#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <vector>
#include <Ws2tcpip.h>
#include "WSAManager.h"
#include <thread>
#include <set>
#include <mutex>
#include <queue>
#include <unordered_map>
#include "Logger.h"

class Scanner
{
public:

	Scanner();

	void ScanPorts(const std::string& host, int minPort, int maxPort, bool fastScan);

	void StopScan();

	std::vector<int> GetOpenedPorts(std::string ip);

	void ScanLocalIps(std::string host, int minPort, int maxPort);

	std::set<std::string> GetLocalIps();

	~Scanner();

	// «апрещаем копирование и присваивание
	Scanner(const Scanner&) = delete;
	Scanner& operator=(const Scanner&) = delete;

	Scanner(Scanner&&) = delete;
	Scanner& operator=(Scanner&&) = delete;

private:
	void TrySocketConnect(SOCKET& connect_socket, sockaddr_in& temp, int port);
	bool TryConnectWithTimeout(const std::string& host, int port, int timeout_ms = 500);

	void AddOpenedPort(sockaddr_in& addr, int port);
	void PrintScanInfo(std::string& s, bool isFirstPrint = true);

	void WaitForAllThreads();

	std::unordered_map<std::string, std::set<int>> localIps;

	std::atomic<bool> stop_scan{ false };
	std::mutex results_mutex;
	std::mutex port_threads_mutex;
	std::vector<std::thread> ip_threads;
	std::vector<std::thread> port_threads;

	// ќграничиваем максимальное количество потоков
	const size_t MAX_IP_THREADS = 50;
	const size_t MAX_PORT_THREADS = 300;
};

