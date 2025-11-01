#include "api.h"

API_request IAPI::ParseToApi(const std::string& req)
{
	int start = 0, index = req.find(separator, start);
	std::string splt;
	API_request api;
	try
	{
		if (index == std::string::npos) throw 1;
		/*if (index == std::string::npos) 
			index = min(req.size(), (req.find('\0') == std::string::npos ? req.size() : req.find('\0')));*/
		splt = req.substr(start, index - start);
		start = index + strlen(separator);
		api.action = static_cast<API>(std::stoi(splt));
	}
	catch (...)
	{
		api.action = invalid_connect;
	}

	for (index = req.find(separator, start);
		index != std::string::npos;
		index = req.find(separator, start))
	{
		splt = req.substr(start, index - start);
		start = index + strlen(separator);
		api.args.push_back(splt);
	}

	//index = min(req.size(), (req.find('\0') == std::string::npos ? req.size() : req.find('\0')));
	//if (start != index) {
	//	api.args.push_back(req.substr(start, index-start));
	//}

	return api;
}

std::string IAPI::ParseToString(const API_request& req)
{
	std::string str = "";
	str += std::to_string(req.action);
	for (int i = 0; i < req.args.size(); i++) str += separator + req.args[i];

	return str + separator;
}

bool client_info::operator==(const client_info& client) const
{
	if (name == client.name && client_socket == client.client_socket)
		return true;
	else return false;
}