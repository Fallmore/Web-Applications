#include "Dns.h"

std::vector<std::string> Dns::GetIpFromDns(std::string name)
{
	struct addrinfo hints,* dnsInfo, * temp;
	std::vector<std::string> result;
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

	// Получаем информацию о сервере
	int status = getaddrinfo(name.c_str(), nullptr, &hints, &dnsInfo);
	if (status != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
        return std::vector<std::string>();
	}

    for (temp = dnsInfo; temp != nullptr; temp = temp->ai_next) {
        std::cout << "Каноническое имя: ";
        if (temp->ai_canonname) std::cout << temp->ai_canonname;
        std::cout << "\n";
        assert(temp->ai_family == temp->ai_addr->sa_family);
        std::cout << "Тип адреса: ";

		void* addr;
		if (temp->ai_family == AF_INET) {
			// Это – адрес IPv4 длиной максимум INET_ADDRSTRLEN.
			std::array<char, INET_ADDRSTRLEN> ip;

			sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(temp->ai_addr);
			addr = &(sin->sin_addr);
			struct sockaddr_in* ipv4 = (struct sockaddr_in*)temp->ai_addr;
			addr = &(ipv4->sin_addr);
			std::cout
				<< "AF_INET\n"
				<< "Длина адреса: "
				<< sizeof(sin->sin_addr) << "\n";
			std::cout
				<< "IP-адрес: "
				<< inet_ntop(AF_INET, &(sin->sin_addr), ip.data(), ip.size())
				<< "\n";
		}
		else {
			// Это адрес IPv6.
			std::array<char, INET6_ADDRSTRLEN> ip6;
			
			// Для хранения адреса IPv6 используется sockaddr_in6.
			sockaddr_in6* sin = reinterpret_cast<sockaddr_in6*>(temp->ai_addr);
			addr = &(sin->sin6_addr);

			std::cout
				<< "AF_INET6\n"
				<< "Длина адреса: " << sizeof(sin->sin6_addr) << "\n"
				<< "IP-адрес: "
				<< inet_ntop(AF_INET6, &(sin->sin6_addr), ip6.data(),
					ip6.size())
				<< "\n";
		}
		std::cout << std::endl;

		inet_ntop(temp->ai_family, addr, ipstr, sizeof ipstr);
		result.push_back(ipstr);
	}
	std::cout << std::endl;

	freeaddrinfo(dnsInfo);
	return result;
}

Dns::Dns()
{
    WSAManager::InitializeWinsock();
}

Dns::~Dns()
{
    WSACleanup();
}
