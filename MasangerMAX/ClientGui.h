#pragma once
#include "Client.h"

using namespace std;

class ClientGui
{
public:
	void Start();

	void ConnectionMenu();

	bool RegistrationMenu();

	bool StartMenu();

	bool CommonChatMenu();

	void CatchMessages();

	void CheckEnter(int& choice, int NumberOfChoice);

private:
	void ClientStartListening();

	Client client;
	string client_name;
	API_request request;
};

