#include "ClientGui.h"

void ClientGui::Start()
{
	bool err = false;
	thread t;
	do
	{
		if (err) {
			cout << "\n\nПотеряно соединение с сервером!";
			err = false;
			system("cls");
			system("pause");
		}

		ConnectionMenu();
		t = thread(&ClientGui::ClientStartListening, this);
		err = RegistrationMenu();
		if (!err) err = StartMenu();

		// Пользователь выходит из приложения
		if (!err) {
			break;
		}
	} while (err);


	client.~Client();
	if (t.joinable())
		t.join();
}

void ClientGui::ConnectionMenu()
{
	std::string host, port, trash;
	int err = 0;

	cout << "Подключение к серверу MessengerMAX\n\n";
	do
	{
		if (err == INVALID_SOCKET) cout << "Ошибка подключения! Введите данные повторно";
		err = 0;

		cout << "Введите IP сервера: ";
		cin >> host;
		cout << "Введите порт сервера: ";
		cin >> port;
		getline(cin, trash);

	} while (err = client.CreateConnection(host, port) == INVALID_SOCKET);

	cout << "\nСоединение с сервером MessengerMAX прошло успешно\n";
	system("pause");
	system("cls");
}

bool ClientGui::RegistrationMenu()
{
	cout << R"(Регистрация в MessengerMAX

Введите свой никнейм: )";
	getline(cin, client_name);

	request.action = register_yourself;
	request.args.clear();
	request.args.push_back(client_name);

	if (!client.SendRequest(request)) return false;

	cout << "\n\nОжидание ответа от сервера...\n\n";

	while (!client.is_response_get) {
		this_thread::sleep_for(200ms);
	}
	if (!client.continue_listening) return false;

	return true;
}

bool ClientGui::StartMenu()
{
	cout << "Добро пожаловать в MessengerMAX, " << client_name << endl;

	cout << R"(Выберите чат для общения
1. Общий чат
2. Групповые чаты
3. Личные чаты

4. Выйти из приложения

Ввод: )";

	int choice = -1;
	CheckEnter(choice, 4);

	system("cls");
	switch (choice)
	{
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		system("pause");
		return false;
		break;
	}
	system("pause");
	system("cls");
	StartMenu();
}

bool ClientGui::CommonChatMenu()
{
	cout << "=====Добро пожаловать в общий чат=====";

	return false;
}

void ClientGui::CatchMessages()
{

}

void ClientGui::CheckEnter(int& choice, int NumberOfChoice)
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

void ClientGui::ClientStartListening()
{
	client.StartListening();
}
