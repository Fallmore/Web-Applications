#pragma once

#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <vector>
#include <string>
#include <iostream>
#include "WSAManager.h"
#include <cassert>
#include <array>

class Dns
{
public:
	std::vector<std::string> GetIpFromDns(std::string name);

	Dns();
	~Dns();
};

