#include "ClientGui.h"

void ClientGui::Start()
{
	bool err = false;
	thread t;
	do
	{
		if (err) {
			cout << "\n\nПотеряно соединение с сервером!\n\n";
			err = false;
			client.Disconnect();
			if (t.joinable())
				t.join();
			system("pause");
			system("cls");
		}

		ConnectionMenu();
		t = thread(&ClientGui::ClientStartListening, this);
		err = !RegistrationMenu();
		if (!err) err = !StartMenu();

		// Пользователь выходит из приложения
		if (!err) {
			break;
		}
	} while (err);


	client.Disconnect();
	if (t.joinable())
		t.join();
}

void ClientGui::ConnectionMenu()
{
	std::string host = "10.178.229.253", port = "6", trash;
	int err = 0;

	cout << "Подключение к серверу MessengerMAX\n\n";
	do
	{
		if (err == INVALID_SOCKET) cout << "Ошибка подключения! Введите данные повторно";
		err = 0;

		//cout << "Введите IP сервера: ";
		//cin >> host;
		//cout << "Введите порт сервера: ";
		//cin >> port;
		//getline(cin, trash);

	} while (err = client.CreateConnection(host, port) == INVALID_SOCKET);

	cout << "\nСоединение с сервером MessengerMAX прошло успешно\n\n";
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
	return WaitResponse(false);
}

bool ClientGui::StartMenu()
{
	system("cls");
	cout << "=====Добро пожаловать в MessengerMAX, " << client_name << "=====\n\n";

	cout << R"(Выберите чат для общения
1. Общий чат
2. Групповые чаты
3. Личные чаты
4. Показать список пользователей

5. Выйти из MessengerMAX

Ввод: )";

	int choice = -1;
	bool err = false;
	API_request request;
	CheckEnter(choice, 5);

	system("cls");
	switch (choice)
	{
	case 1:
		request.action = send_message_in_common_chat;
		err = !ChatMenu(request);
		break;
	case 2:
		request.action = send_message_in_group_chat;
		err = !ChooseChat(request);
		break;
	case 3:
		request.action = send_message_in_p2p_chat;
		err = !ChooseChat(request);
		break;
	case 4:
		err = !ShowUserList();
		break;
	case 5:
		return true;
		break;
	}
	if (err) return false;

	return StartMenu();
}

bool ClientGui::ChooseChat(API_request& request)
{
	int i = 0, choice = -1;
	bool is_group_chat = false;
	client_info member;
	chats chats;
	do
	{
		chats = client.GetChats();
		system("cls");
		cout << "=====Выберите чат=====\n\n";

		if (request.action == send_message_in_group_chat) {
			is_group_chat = true;
			for (i = 0; i < chats.group_chats.size(); i++)
			{
				cout << i + 1 << ". " << chats.group_chats[i].name << endl;
			}
		}
		else if (request.action == send_message_in_p2p_chat) {
			is_group_chat = false;
			for (i = 0; i < chats.p2p_chats.size(); i++)
			{
				member = chats.p2p_chats[i].member1 == client.we
					? chats.p2p_chats[i].member2
					: chats.p2p_chats[i].member1;

				cout << i + 1 << ". " << member.name << endl;
			}
		}

		cout << endl << ++i << ". Создать чат\n";
		cout << ++i << ". Вернуться\n\nВвод: ";

		CheckEnter(choice, i);

		if (choice == i - 1) {
			if (!CreateChat(request)) return false;
		}
	} while (choice == i - 1);

	if (choice == i) return true;
	--choice;

	request.args.clear();
	if (is_group_chat) {
		request.args.push_back(client.GetChats().group_chats[choice].name);
	}
	else {
		member = chats.p2p_chats[choice].member1 == client.we
			? chats.p2p_chats[choice].member2
			: chats.p2p_chats[choice].member1;
		request.args.push_back(std::to_string(member.client_socket));
		request.args.push_back(member.name);
	}

	return ChatMenu(request);
}

bool ClientGui::CreateChat(API_request& request)
{
	string welcome, name, waiting = "Получение списка пользоваталей...";
	vector<string> local_cl_name;
	bool multiple_choice = false;
	API_request temp_req;
	temp_req.action = request.action == send_message_in_group_chat ? create_group_chat : create_p2p_chat;
	int i = 0, choice = -1;

	system("cls");
	cout << waiting;
	if (!GetUserList()) return false;
	printf("\33[2K\r");
	local_cl_name = vector<string>(clients_name);

	switch (temp_req.action)
	{
	case create_group_chat:
		welcome = "группового чата";
		multiple_choice = true;
		break;
	case create_p2p_chat:
		welcome = "личного чата";
		for (auto& cl_name : clients_name_p2p_chat)
		{
			local_cl_name.erase(remove(local_cl_name.begin(), local_cl_name.end(), cl_name),
				local_cl_name.end());
		}
		break;
	default:
		throw 1;
		break;
	}

	cout << "=====Создание " + welcome + "=====\n";
	if (multiple_choice) {
		cout << "\nНапишите название группового чата: ";
		getline(cin, name);
		temp_req.args.push_back(name);
	}

	cout << "\nВыберите, с кем хотите общаться:\n";

	if (!local_cl_name.empty()) {
		for (i = 0; i < local_cl_name.size(); i++) {
			cout << i + 1 << ". " << local_cl_name[i] << endl;
		}
	}
	else {
		cout << "Доступных пользователей нет\n";
	}
	cout << endl << ++i << ". Вернуться\n\nВвод: ";
	CheckEnter(choice, i);
	if (choice == i) return true;
	choice--;

	temp_req.args.push_back(local_cl_name[choice]);

	if (multiple_choice) {
		do
		{
			auto it = remove(local_cl_name.begin(), local_cl_name.end(), local_cl_name[choice]);
			local_cl_name.erase(it);
			system("cls");
			for (i = 0; i < local_cl_name.size(); i++) {
				cout << i + 1 << ". " << local_cl_name[i] << endl;
			}
			cout << endl << ++i << ". Закончить\n";
			cout << ++i << ". Вернуться\n\nВвод: ";
			CheckEnter(choice, i);
			choice--;
			if (choice == i - 1) return true;
			if (choice == i - 2) break;

			temp_req.args.push_back(local_cl_name[choice]);

		} while (choice != i - 2);
	}

	if (!client.SendRequest(temp_req)) return false;
	if (!multiple_choice) clients_name_p2p_chat.push_back(local_cl_name[choice]);
	API_request response;
	// Если кто-то что-то отправил в какой-то чат, то ждём, 
	// пока не пришлют ответ на создание чата
	do
	{
		system("cls");
		cout << "\n\nОжидание ответа от сервера...\n\n";
		if (!WaitResponse(false)) return false;
		response = client.GetResponse();
	} while (response.action != temp_req.action && response.action != error);

	if (response.action == temp_req.action) {
		cout << "\nЧат успешно создан\n";
	}
	cout << endl;
	system("pause");

	return true;
}

bool ClientGui::ChatMenu(API_request& request)
{
	system("cls");
	cout << "=====Добро пожаловать в " + GetChatName(request) + "=====\n";

	cout << R"(
1. Открыть чат
2. Открыть файлы

3. Вернуться

Ввод: )";

	int choice = -1;
	bool err = false;
	CheckEnter(choice, 3);

	system("cls");
	switch (choice)
	{
	case 1:
		err = !OpenMessages(request);
		break;
	case 2:
		err = !OpenFiles(request);
		break;
	case 3:
		return true;
		break;
	}
	if (err) return false;

	return ChatMenu(request);
}

bool ClientGui::OpenMessages(API_request& request)
{
	common_chat chat;
	atomic<bool> update_messages = false,
		err = false;
	thread userInput;
	cancel = false;

	while (!cancel && !err) {
		system("cls");
		cout << "=====" + GetChatName(request) + "=====\n\n";

		chat = client.GetChat(request);
		for (auto& message : chat.messages) cout << message << endl;

		cout << "\n1. Вернуться\n\nВвод: ";

		if (userInput.joinable()) {
			cancel = true;
			userInput.join();
			cancel = false;
		}
		update_messages = false;
		userInput = thread(&ClientGui::WriteMessages, this,
			ref(request), ref(update_messages), ref(cancel), ref(err));

		while (!update_messages && !cancel && !err) {
			if (!WaitResponse(false)) {
				err = true;
				break;
			}

			API_request response = client.GetResponse();
			// Проверяем, пришло ли сообщение в наш чат
			if (response.action == request.action) {
				if (response.action == send_message_in_group_chat &&
					request.args[0] != response.args[0]) continue;
				else if (response.action == send_message_in_p2p_chat &&
					request.args[0] != response.args[0] &&
					request.args[1] != response.args[1]) continue;

				update_messages = true;
			}
			// Проверяем, отправил ли кто файл в этот чат
			else if (response.action != request.action) {
				if (response.action == send_file_in_common_chat
					|| (response.action == send_file_in_group_chat
						&& request.args[0] == response.args[0])
					|| (response.action == send_file_in_p2p_chat &&
						request.args[0] == response.args[0] &&
						request.args[1] == response.args[1]))
					update_messages = true;
			}
		}
	}

	if (userInput.joinable()) {
		cancel = true;
		userInput.join();
		while (!cancel)
			this_thread::sleep_for(100ms);
	}

	// Если даже cancel = true, то всё равно возврат !err отлично подходит
	return !err;
}

void ClientGui::WriteMessages(API_request& request, atomic<bool>& writed,
	atomic<bool>& stop_write, atomic<bool>& err)
{
	string message;
	API_request temp = request;
	while (!writed && !stop_write && !err) {
		if (_kbhit() == 0) {
			this_thread::sleep_for(50ms);
			continue;
		}

		getline(cin, message);

		if (message == "1") {
			stop_write = true;
		}

		if (!message.empty() && !err && !stop_write) {
			temp.args.push_back(message);
			cout << "\nПодождите отправку... ";

			if (!client.SendRequest(temp)) err = true;
		}
	}
}

bool ClientGui::OpenFiles(API_request& request)
{
	API_request temp_req = request;
	common_chat chat;
	int i = 0, choice = -1;
	do
	{
		chat = client.GetChat(request);
		system("cls");
		cout << "=====" + GetChatName(request) + "=====\n\n";
		cout << "Выберите, какой файл скачать:\n";


		if (request.action == send_message_in_common_chat)
			temp_req.action = get_file_from_common_chat;
		else if (request.action == send_message_in_group_chat)
			temp_req.action = get_file_from_group_chat;
		else
			temp_req.action = get_file_from_p2p_chat;

		for (i = 0; i < chat.file_paths.size(); i++)
		{
			cout << i + 1 << ". " << chat.file_paths[i] << endl;
		}

		cout << endl << ++i << ". Отправить файл\n";
		cout << ++i << ". Вернуться\n\nВвод: ";

		CheckEnter(choice, i);

		if (choice == i - 1) {

			if (request.action == send_message_in_common_chat)
				temp_req.action = send_file_in_common_chat;
			else if (request.action == send_message_in_group_chat)
				temp_req.action = send_file_in_group_chat;
			else
				temp_req.action = send_file_in_p2p_chat;

			if (!SendFile(temp_req)) return false;
		}
	} while (choice == i - 1);

	if (choice == i) return true;
	--choice;

	temp_req.args.clear();
	temp_req.args.push_back(chat.file_paths[choice]);

	if (!client.SendRequest(temp_req)) return false;
	API_request response;
	// Если кто-то что-то отправил в какой-то чат, то ждём, 
	// пока не пришлют ответ на скачивание файла
	do
	{
		system("cls");
		cout << "\n\nОжидание ответа от сервера...\n\n";
		if (!WaitResponse(false)) return false;
		response = client.GetResponse();
	} while (response.action != temp_req.action && response.action != error);

	if (response.action == temp_req.action) {
		cout << "\nФайл успешно скачан\n";
	}
	cout << endl;
	system("pause");

	return true;
}

bool ClientGui::SendFile(API_request& request)
{
	system("cls");
	cout << "=====Отправка файла=====\n\n";

	cout << "Введите путь отправляемого файла: ";
	string file_path;
	getline(cin, file_path);
	API_request temp_req = request;
	temp_req.args.push_back(file_path);

	if (!client.SendRequest(temp_req)) return false;
	API_request response;
	// Если кто-то что-то отправил в какой-то чат, то ждём, 
	// пока не пришлют ответ на скачивание файла
	do
	{
		system("cls");
		cout << "\n\nОжидание ответа от сервера...\n\n";
		if (!WaitResponse(false)) return false;
		response = client.GetResponse();
	} while (response.action != temp_req.action && response.action != error);

	if (response.action == temp_req.action) {
		cout << "\nФайл успешно отправлен\n\n";
	}
	cout << endl;
	system("pause");

	return true;
}

bool ClientGui::GetUserList()
{
	API_request api = { show_client_list , {} };
	if (!client.SendRequest(api)) return false;

	API_request response;
	// Если кто-то что-то отправил в какой-то чат, то ждём, 
	// пока не пришлют список
	do
	{
		if (!WaitResponse(false)) return false;
		response = client.GetResponse();
	} while (response.action != show_client_list);

	ParseUserList(response.args[0]);
	return true;
}

void ClientGui::ParseUserList(string& list)
{
	string name, sublist = list;
	int start = 0, index_comma = sublist.find(','), index_eof = sublist.find('\n');

	clients_name.clear();
	for (;
		index_comma != string::npos && index_eof != string::npos;
		index_comma = sublist.find(','), index_eof = sublist.find('\n'))
	{
		name = sublist.substr(start, index_comma - start);
		if (name != client.we.name)
			clients_name.push_back(name);
		if (++index_eof == sublist.size()) break;
		sublist = sublist.substr(index_eof, sublist.size() - index_eof);
	}
}

bool ClientGui::ShowUserList()
{
	API_request api = { show_client_list , {} };
	if (!client.SendRequest(api)) return false;

	API_request response;
	// Если кто-то что-то отправил в какой-то чат, то ждём, 
	// пока не пришлют список
	do
	{
		system("cls");
		cout << "\n\nОжидание ответа от сервера...\n\n";
		if (!WaitResponse(false)) return false;
		response = client.GetResponse();
	} while (response.action != show_client_list && response.action != error);

	system("cls");
	cout << "=====Список пользователей=====\n\n";
	cout << response.args[0] << endl;
	system("pause");
	return true;
}

void ClientGui::ClientStartListening()
{
	client.StartListening();
}

std::string ClientGui::GetChatName(API_request& request)
{
	std::string res;
	switch (request.action)
	{
	case send_message_in_common_chat:
		res = "общий чат";
		break;
	case send_message_in_group_chat:
		res = "групповой чат " + request.args[0];
		break;
	case send_message_in_p2p_chat:
		res = "личный чат с " + request.args[1];
		break;
	default:
		break;
	}
	return res;
}

bool ClientGui::WaitResponse(bool withTimeout)
{
	if (withTimeout) {
		int cnt = 15;
		while (!client.is_response_get.load(std::memory_order_acquire) && cnt != 0) {
			cnt--;
			this_thread::sleep_for(500ms);
		}
		if (cnt == 0) return false;
	}
	else {
		while (!client.is_response_get.load(std::memory_order_acquire) && !cancel && client.continue_listening) {
			this_thread::sleep_for(500ms);
		}
	}
	if (!client.continue_listening) return false;

	return true;
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