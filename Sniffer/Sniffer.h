#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <Ws2tcpip.h>
#include "WSAManager.h"
#include "pcap.h"
#include <array>
#include <chrono>
#include <fstream>
#include <mstcpip.h>
#include <iphlpapi.h>

// ƒлина названи€ интерфейса в структуре ниже, как правило Ч 16.
#define IFNAMSIZ 16
#define ethernet_proto_type_offset 2 * 6

class Sniffer
{
public:
	bool init(std::string path_pcap, std::string if_address);
	bool bind_socket();
	bool switch_promisc(bool turn_on);
	bool capture();
	bool start_capture();
	void stop_capture();
	bool is_capturing();

	Sniffer();
	~Sniffer();

private:
	sockaddr_in get_if_address(const std::string& if_name, int sock_);

	SOCKET sock_;
	bool initialized_ = false;
	std::atomic<bool> started_ = false;
	std::string if_address_;
	std::ofstream of_;
};

