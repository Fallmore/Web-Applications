#pragma once
#include <cstdint>

constexpr auto PCAP_VERSION_MAJOR = 2;
constexpr auto PCAP_VERSION_MINOR = 4;
constexpr auto DLT_EN10MB = 1;
// Полный размер буфера пакета.
constexpr auto BUFFER_SIZE_PKT = 256 * 256 - 1;

struct pcap_timeval
{
	// UNIX time_t.
	int32_t tv_sec;
	// Смещение в микросекундах от tv_sec.
	int32_t tv_usec;
};

// Заголовок PCAP-файла.
struct pcap_file_header
{
	// "Магическое число".
	const uint32_t magic = 0xa1b2c3d4;
	// Главная версия.
	const uint16_t version_major = PCAP_VERSION_MAJOR;
	// Минорная версия.
	const uint16_t version_minor = PCAP_VERSION_MINOR;
	// Локальная временная зона для коррекции времени относительно GMT.
	int32_t thiszone = 0;
	// Точность временной метки.
	uint32_t sigfigs = 0;
	// Максимальная длина захватываемых пакетов в октетах.
	uint32_t snaplen = BUFFER_SIZE_PKT;
	// Тип соединения.
	uint32_t linktype = DLT_EN10MB;
};

struct pcap_sf_pkthdr
{
	pcap_timeval ts;
	// Количество октетов пакета, сохраненных в файле.
	uint32_t caplen;
	// Реальная длина пакета.
	uint32_t len;
};

// Размер PCAP-заголовка пакета.
constexpr auto BUFFER_SIZE_HDR = sizeof(pcap_sf_pkthdr);
// Размер Ethernet-заголовка.
constexpr auto BUFFER_SIZE_ETH = 14;
// Размер IP-пакета.
constexpr auto BUFFER_SIZE_IP = BUFFER_SIZE_PKT - BUFFER_SIZE_ETH;
// Смещение Ethernet-заголовка в буфере: сразу за PCAP-заголовком.
constexpr auto BUFFER_OFFSET_ETH = sizeof(pcap_sf_pkthdr);
// Смещение IP-заголовка в буфере.
constexpr auto BUFFER_OFFSET_IP = BUFFER_OFFSET_ETH + BUFFER_SIZE_ETH;

#if defined(WIN32)
// Для ОС Windows Ethernet-заголовок отсутствует.
constexpr auto BUFFER_WRITE_OFFSET = BUFFER_OFFSET_IP;
constexpr auto BUFFER_ADD_HEADER_SIZE = BUFFER_SIZE_ETH;
#else
constexpr auto BUFFER_WRITE_OFFSET = BUFFER_OFFSET_ETH;
constexpr auto BUFFER_ADD_HEADER_SIZE = BUFFER_SIZE_ETH;
#endif
