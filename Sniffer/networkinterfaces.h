#pragma once
#include <winsock2.h>
#include <iphlpapi.h>
#include <iostream>

#pragma comment(lib, "iphlpapi.lib")

void print_network_interfaces();