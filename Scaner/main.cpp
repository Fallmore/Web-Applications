#include <iostream>
#include "Scanner.h";

using namespace std;

void ManageScanningPorts(Scanner& scanner) {
	string host, minPort, maxPort;
	set<string> ips = scanner.GetLocalIps();

	if (ips.size() != 0) {
		cout << "Доступные локальные IP-адреса:\n";
		copy(ips.begin(), ips.end(),
			ostream_iterator<string>(cout, "	\n"));
	}

	cout << "Введите IP-адрес сканирования портов: ";
	cin >> host;
	cout << "Введите минимальный и максимальный порты (default: 0, 65535): ";
	cin >> minPort >> maxPort;

	int min = minPort == "" ? 0 : stoi(minPort),
		max = maxPort == "" ? 0 : stoi(maxPort);

	cout << "\nСканирование портов...\n\n";
	scanner.ScanPorts(host, min, max, false);
	vector<int> op = scanner.GetOpenedPorts(host);
	cout << "\nОткрытые порты:\n";
	copy(op.begin(), op.end(),
		ostream_iterator<int>(cout, "	\n"));
}

void ManageScanningLocalIps(Scanner& scanner) {
	string host, minPort, maxPort;

	cout << "Введите локальный IP-адрес (например, 192.168.1): ";
	cin >> host;
	cout << "Введите минимальный и максимальный порты (default: 0, 65535): ";
	cin >> minPort >> maxPort;

	int min = minPort == "" ? 0 : stoi(minPort),
		max = maxPort == "" ? 0 : stoi(maxPort);

	cout << "\nСканирование IP-адресов...\n";
	scanner.ScanLocalIps(host, min, max);
	set<string> ips = scanner.GetLocalIps();
	cout << "\nНайденные локальные IP-адреса:\n";
	copy(ips.begin(), ips.end(),
		ostream_iterator<string>(cout, "	\n"));

}


void CheckEnter(int& choice, int NumberOfChoice)
{
	cin.clear();
	int rep = 0;
	do
	{
		if (!(cin >> choice))
		{
			if (rep < 1) cout << "\nДействие отсутствует" << endl << "Введите повторно: ";
			rep = 1;
		}
		else if (choice > NumberOfChoice || choice < 0)
		{
			if (rep < 1) cout << "\nДействие отсутствует" << endl << "Введите повторно: ";
			rep = 1;
		}
	} while (getchar() != '\n');
	if (rep != 0) CheckEnter(choice, NumberOfChoice);

}

void MainMenu(Scanner& scanner) {
	cout << R"(Лабораторная работа по Сетевым приложениям №3-4

1. Сканирование портов
2. Сканирование сети

0. Завершить программу
)";

	set<string> ips = scanner.GetLocalIps();
	if (ips.size() != 0) {
		cout << "\nДоступные локальные IP-адреса:\n";
		copy(ips.begin(), ips.end(),
			ostream_iterator<string>(cout, "	\n"));
	}

	cout << "\nВвод: ";

	int choice = -1;
	CheckEnter(choice, 3);

	system("cls");
	switch (choice)
	{
	case 1:
		ManageScanningPorts(scanner);
		break;
	case 2:
		ManageScanningLocalIps(scanner);
		break;
	case 0:
		return;
		break;
	}
	system("pause");
	system("cls");
	MainMenu(scanner);
}

int main()
{
	SetConsoleOutputCP(1251);
	SetConsoleCP(1251);
	setlocale(LC_ALL, "RUS");

	Scanner scanner;
	MainMenu(scanner);
	//delete(scanner);
	system("pause");
}