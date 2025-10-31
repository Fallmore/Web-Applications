#include "api.h"

API_request IAPI::ParseToApi(const std::string& req)
{
	char* next_token1 = NULL;
	char* splt = strtok_s(const_cast<char*>(req.c_str()), separator, &next_token1);
	API_request api;
	try
	{
		if (splt == nullptr) throw 1;
		api.action = static_cast<API>(std::stoi(splt));
	}
	catch (...)
	{
		api.action = invalid_connect;
	}

	splt = strtok_s(nullptr, separator, &next_token1);
	while (splt != nullptr)
	{
		api.args.push_back(splt);
		splt = strtok_s(nullptr, separator, &next_token1);
	}

	return api;
}

std::string IAPI::ParseToString(const API_request& req)
{
	std::string str = "";
	str += std::to_string(req.action);
	for (int i = 0; i < req.args.size(); i++) str += separator + req.args[i];

	return str;
}

bool client_info::operator==(const client_info& client) const
{
	if (name == client.name && client_socket == client.client_socket)
		return true;
	else return false;
}