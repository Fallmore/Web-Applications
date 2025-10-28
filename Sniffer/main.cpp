#include "Sniffer.h"
#include <thread>
#include <string>
#include "networkinterfaces.h"

using namespace std;

Sniffer sniffer;
std::thread t;
atomic<bool> stopped = false;

void StopSniff() {
	stopped = true;
	sniffer.stop_capture();
	if (t.joinable())
		t.join();
}

void StartSniff() {
	sniffer.start_capture();
}

void MainMenu(Sniffer& sniffer) {
	std::string addr, path;
	cout << R"(Лабораторная работа по Сетевым приложениям №5

Введите адрес прослушивания: )";
	cin >> addr;
	cout << "Введите путь/название файла записи пакетов: ";
	getline(cin, path);
	getline(cin, path);

	if (!stopped)
		if (sniffer.init(path, addr)) {
			t = std::thread(StartSniff);

			if (t.joinable()) {
				t.join();
			}
		}

	system("pause");
	system("cls");
	if (!stopped)
		MainMenu(sniffer);
}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
		// Handle the CTRL-C signal.
	case CTRL_C_EVENT:
		if (sniffer.is_capturing())
			StopSniff();
		else {
			stopped = true;
			sniffer.~Sniffer();
			exit(0);
		}
		return TRUE;

	default:
		return FALSE;
	}
}

int main()
{
	SetConsoleOutputCP(1251);
	SetConsoleCP(1251);
	setlocale(LC_ALL, "RUS");

	if (SetConsoleCtrlHandler(CtrlHandler, TRUE))
	{
		print_network_interfaces();
		MainMenu(sniffer);
		/*thread t = thread(MainMenu, sniffer);
		if (t.joinable())
			t.join();*/
	}
	else
	{
		printf("\nERROR: Could not set control handler");
		return 1;
	}
	return 0;



	system("pause");
}