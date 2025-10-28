// HttpFileDns.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#pragma comment(lib, "wininet.lib")
#include <iostream>
#include "Http.h"
#include "Dns.h"
#include <array>
#include <wininet.h>

using namespace std;

void HandleHttp() {
	Http http;
	string addr, body = "", method;
	Http::HttpMethod meth;

	cout << "Введите адрес: ";
	cin >> addr;
	cout << "Введите метод (GET, PUT): ";
	cin >> method;

	for (auto& x : method) {
		x = toupper(x);
	}

	if (method == "GET") {
		meth = Http::HttpMethod::GET_;
	}
	else if (method == "PUT") {
		meth = Http::HttpMethod::PUT_;
		cout << "Введите тело запроса: ";
		std::getline(cin, body);
		std::getline(cin, body);
	}
	else {
		meth = Http::HttpMethod::GET_;
	}

	http.HandleHTTP(addr, meth, body);

	http.~Http();

}

void HandleFile() {
	Http http;
	string filePath, addr = "http://localhost:8888/";
	Http::HttpMethod method = Http::HttpMethod::PUT_;

	cout << "Введите путь к файлу: " << "file:///";
	getline(cin, filePath);

	http.HandleHTTP(addr, method, "file:///" + filePath);
	http.~Http();
}

void HandleDns() {
	Dns dns;
	string name;
	vector<string> ips;

	cout << "Введите DNS: ";
	cin >> name;

	ips = dns.GetIpFromDns(name);
	cout << endl << "IP-адреса домена " << name << ":" << endl;

	if (ips.size() == 0) cout << "IP-адреса отсутствуют" << endl;
	else {
		for (auto ip : ips)
		{
			cout << ip << endl;
		}
		cout << endl;
	}

	dns.~Dns();
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
	cout << R"(Лабораторная работа по Сетевым приложениям №1

Выдача ответа на запрос
1. По протоколу http
2. По протоколу file
3. DNS

4. Завершить программу

Ввод: )";

	int choice = -1;
	CheckEnter(choice, 4);

	system("cls");
	switch (choice)
	{
	case 1:
		HandleHttp();
		break;
	case 2:
		HandleFile();
		break;
	case 3:
		HandleDns();
		break;
	case 4:
		system("pause");
		exit(0);
		break;
	}
	system("pause");
	system("cls");
	MainMenu();
}

int main()
{
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	setlocale(LC_ALL, "RUS");

	MainMenu();



	//const auto internet_handle = InternetOpen(TEXT("YaBrowser/25.8.0.0"),
	//	NULL, nullptr, nullptr, 0);
	//if (!internet_handle)
	//{
	//	std::cerr
	//		<<
	//		"InternetOpen failed with code " << GetLastError()
	//		<<
	//		std::endl;
	//	return EXIT_FAILURE;
	//}

	//// Открыть ресурс.
	//auto url_handle = InternetOpenUrl(internet_handle,
	//	TEXT("https://ru.wikipedia.org/wiki"), 0, 0,
	//	INTERNET_FLAG_RAW_DATA, 0);
	//if (!url_handle)
	//{
	//	std::cerr <<"InternetOpenUrl failed with code " << GetLastError() << std::endl;
	//	InternetCloseHandle(internet_handle);
	//	return EXIT_FAILURE;
	//}

	//char cBuffer[1024];            // I'm only going to access 1K of info.
	//BOOL bResult;
	//DWORD dwBytesRead;
	//// Прочитать данные.
	//while (InternetReadFile(url_handle, cBuffer,
	//	(DWORD)1024, &dwBytesRead))
	//{
	//	if (!dwBytesRead) break;
	//	std::cout << cBuffer;
	//}

	//// Закрыть дескриптор ресурса.
	//InternetCloseHandle(url_handle);
	//// Закрыть дескриптор интернета.
	//InternetCloseHandle(internet_handle);
	//return EXIT_SUCCESS;
}
