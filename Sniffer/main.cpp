#include "Sniffer.h"
#include <thread>
#include <string>
#include "networkinterfaces.h"

using namespace std;

void MainMenu(Sniffer& sniffer) {
	std::string addr, path;
	print_network_interfaces();
	cout << R"(Лабораторная работа по Сетевым приложениям №5

Введите адрес прослушивания: )";
	cin >> addr;
	cout << "Введите путь/название файла записи пакетов: ";
	getline(cin, path);
	getline(cin, path);

	if (sniffer.init(path, addr)) {
		sniffer.start_capture();
	}

	system("pause");
	system("cls");
	MainMenu(sniffer);
}

int main()
{
	SetConsoleOutputCP(1251);
	SetConsoleCP(1251);
	setlocale(LC_ALL, "RUS");

	Sniffer sniffer;
	MainMenu(sniffer);

	system("pause");

	return 0;
}