#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h> // для getaddrinfo
#include <iostream>

class WSAManager
{
public:
	bool static InitializeWinsock();
};

