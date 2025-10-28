#include "Server.h"

int main()
{
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	setlocale(LC_ALL, "RUS");

	Server server;
	server.Start();

	system("pause");
	return 0;
}