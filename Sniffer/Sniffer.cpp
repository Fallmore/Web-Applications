#include "Sniffer.h"

bool Sniffer::init(std::string path_pcap, std::string if_address)
{
	if_address_ = if_address;
	sock_ = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
	if (sock_ == INVALID_SOCKET) {
		std::cerr << "socket() failed: " << WSAGetLastError() << std::endl;
		return false;
	}
	// Для Windows адрес должен быть привязан до переключения в promisc-режим.
	if (!bind_socket() || !switch_promisc(true))
		return false;

	of_ = std::ofstream(path_pcap);
	// Проверка успешного открытия файла
	if (!of_.is_open()) {
		std::cerr << "Не удалось открыть файл!" << std::endl;
		return false;
	}

	initialized_ = true;

	return true;
}

bool Sniffer::bind_socket()
{
	const size_t len = if_address_.size();
	if (IFNAMSIZ <= len)
	{
		std::cerr << "Имя интерфейса слишком длинное!" << std::endl;
		return false;
	}
	// Получить адрес интерфейса.
	const sockaddr_in iface_addr = get_if_address(if_address_, sock_);
	// Привязать сокет к адресу.
	if (-1 == bind(sock_, (sockaddr*)&iface_addr, sizeof(iface_addr)))
	{
		std::cerr << "bind() failed: " << WSAGetLastError() << "." << std::endl;
		return false;
	}
	return true;
}

bool Sniffer::switch_promisc(bool turn_on)
{
	DWORD dwValue = (turn_on ? RCVALL_ON : RCVALL_OFF);
	DWORD dwBytesReturned = 0;
	// Выполнить системный вызов.
	if (SOCKET_ERROR == WSAIoctl(sock_, SIO_RCVALL, &dwValue, sizeof(dwValue),
		nullptr, 0, &dwBytesReturned, nullptr, nullptr))
	{
		std::cerr << "switch_promisc() failed: " << WSAGetLastError() << "." << std::endl;
		return false;
	}
	return true;
}

bool Sniffer::capture()
{
	// Первые 14 байт — фейковый Ethernet-заголовок под протокол IPv4.
	std::array<char, BUFFER_SIZE_HDR + BUFFER_SIZE_PKT> buffer;
	// 0x08 — тип IP в кадре Ethernet. По смещению = 12.
	buffer[BUFFER_OFFSET_ETH] = 0x08;
	pcap_sf_pkthdr* pkt = reinterpret_cast<pcap_sf_pkthdr*>(buffer.data());
	// Прочитать очередной пакет.
	const int rc = recv(sock_, buffer.data() + BUFFER_WRITE_OFFSET, BUFFER_SIZE_IP, 0);
	if (-1 == rc)
	{
		// Ошибка приема данных — перестать читать пакеты.
		std::cerr << "recv() failed: " << WSAGetLastError() << std::endl;
		return false;
	}
	// Соединение разорвано — перестать читать пакеты.
	if (!rc) return false;
	std::cout << rc << " байт получено..." << std::endl;

	using namespace std::chrono;
	// Рассчитать временную метку пакета.
	const auto cur_time = duration_cast<microseconds>(
		time_point_cast<microseconds>(
			system_clock::now()).time_since_epoch()
	);
	const auto t_s = seconds(duration_cast<seconds>(cur_time));
	const auto u_s = cur_time - duration_cast<microseconds>(t_s);

	// Установить поля заголовка PCAP.
	pkt->ts.tv_sec = t_s.count();
	pkt->ts.tv_usec = u_s.count();
	pkt->caplen = rc + BUFFER_ADD_HEADER_SIZE;
	pkt->len = rc + BUFFER_ADD_HEADER_SIZE;

	// Запись пакета в файл.
	of_.write(buffer.data(), rc + BUFFER_SIZE_HDR + BUFFER_ADD_HEADER_SIZE);
	of_.flush();
	return true;
}

bool Sniffer::start_capture()
{
	if (started_ || !initialized_) return false;
	// Переключить адаптер в неразборчивый режим.
	if (!switch_promisc(true)) return false;

	started_ = true;
	std::cout << "Начало захвата на интерфейсе " << if_address_ << std::endl;
	// Начать цикл захвата.
	while (started_)
	{
		if (!capture()) return false;
	}
	return true;
}

void Sniffer::stop_capture()
{
	started_ = false;
}

bool Sniffer::is_capturing()
{
	return started_;
}

Sniffer::Sniffer()
{
	if (!WSAManager::InitializeWinsock()) throw std::exception("Failed initialize winsock");
}

Sniffer::~Sniffer()
{
	if (initialized_) switch_promisc(false);
	closesocket(sock_);
	of_.close();
	WSACleanup();
}

sockaddr_in Sniffer::get_if_address(const std::string& if_address, int sock_)
{
	sockaddr_in sa;
	sa.sin_family = PF_INET;
	sa.sin_port = 0;

	inet_pton(AF_INET, if_address.c_str(), &sa.sin_addr);
	return sa;
}
