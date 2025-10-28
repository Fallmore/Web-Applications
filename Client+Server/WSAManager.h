#pragma once
#include <winsock2.h>
#include <ws2tcpip.h> // для getaddrinfo
#include <iostream>
#pragma comment(lib, "ws2_32.lib")

class WSAManager
{
public:
	bool static InitializeWinsock();
};

