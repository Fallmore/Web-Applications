// HttpFileDns.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "Server.h"
#include "ClientGui.h"
//#include "SplitConsole.h"

using namespace std;

void SettingsServer() {
	Server server;
	string host, portS;
	int port;
	bool err = false;
	do
	{
		if (err) cout << "Ошибка! Введите другие параметры!" << endl;

		cout << "Введите IP-адресс для сервера: ";
		cin >> host;

		do
		{
			cout << "Введите порт для сервера: ";
			cin >> portS;
			try
			{
				port = stoi(portS);
				err = false;
			}
			catch (const std::exception&)
			{
				cout << "Ошибка! Вы ввели сторонние символы!" << endl;
				err = true;
			}

		} while (err);


	} while (err = !server.Start(host, port)); // Если будет ошибка во время старта, то продолжится цикл, 
	//	а иначе сервер будет работать
}

void StartClientGui() {
	ClientGui gui;

	gui.Start();
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
		else if (choice > NumberOfChoice || choice < 1)
		{
			if (rep < 1) cout << "\nДействие отсутствует" << endl << "Введите повторно: ";
			rep = 1;
		}
	} while (getchar() != '\n');
	if (rep != 0) CheckEnter(choice, NumberOfChoice);

}

void MainMenu() {
	cout << R"(Лабораторная работа по Сетевым приложениям №6

Быть
1. Сервером
2. Клиентом

3. Завершить программу

Ввод: )";

	int choice = -1;
	CheckEnter(choice, 3);

	system("cls");
	switch (choice)
	{
	case 1:
		SettingsServer();
		break;
	case 2:
		StartClientGui();
		break;
	case 3:
		system("pause");
		return;
		break;
	}
	system("pause");
	system("cls");
	MainMenu();
}

int main()
{
	SetConsoleOutputCP(1251);
	SetConsoleCP(1251);
	setlocale(LC_ALL, "RUS");

	MainMenu();
	/*system("pause");
	SplitConsole sc;
	
	system("pause");*/
}