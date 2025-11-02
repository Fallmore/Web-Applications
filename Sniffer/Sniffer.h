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

typedef struct IPHeader
{
	unsigned char  ip_header_len : 4;  // 4-bit header length (in 32-bit words) normally=5 (Means 20 Bytes may be 24 also)
	unsigned char  ip_version : 4;  // 4-bit IPv4 version
	unsigned char  ip_tos;           // IP type of service
	unsigned short ip_total_length;  // Total length
	unsigned short ip_id;            // Unique identifier 
	unsigned char  ip_frag_offset : 5;        // Fragment offset field
	unsigned char  ip_more_fragment : 1;
	unsigned char  ip_dont_fragment : 1;
	unsigned char  ip_reserved_zero : 1;

	unsigned char  ip_frag_offset1;    //fragment offset

	unsigned char  ip_ttl;           // Time to live
	unsigned char  ip_protocol;      // Protocol(TCP,UDP etc)
	unsigned short ip_checksum;      // IP checksum
	unsigned int   ip_srcaddr;       // Source address
	unsigned int   ip_destaddr;      // Source address
};

class Sniffer
{
public:
	bool init(std::string path_pcap, std::string if_address);
	bool bind_socket();
	bool switch_promisc(bool turn_on);
	void write_pcap_file_header();
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

