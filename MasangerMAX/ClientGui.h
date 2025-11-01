#pragma once
#include "Client.h"
#include <conio.h>

using namespace std;

class ClientGui
{
public:
	void Start();
private:
	void ConnectionMenu();
	bool RegistrationMenu();
	bool StartMenu();
	bool ChooseChat(API_request& request);
	bool CreateChat(API_request& request);
	bool ChatMenu(API_request& request);
	bool OpenMessages(API_request& request);
	void WriteMessages(API_request& request, atomic<bool>& writed,
		atomic<bool>& stop_write, atomic<bool>& err);
	bool OpenFiles(API_request& request);
	bool SendFile(API_request& request);
	bool GetUserList();
	void ParseUserList(string& list);
	bool ShowUserList();
	void ClientStartListening();
	std::string GetChatName(API_request& request);
	atomic<bool> cancel = false;
	bool WaitResponse(bool withTimeout = true);
	void CheckEnter(int& choice, int NumberOfChoice);

	Client client;
	vector<string> clients_name, clients_name_p2p_chat;
	string client_name;
	API_request request;
};

