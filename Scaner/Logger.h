#pragma once
#include <ctime>
#include <iostream>
#include <string>

class Logger
{
public:
	std::string static LogTime();
	void static Log(std::string message);
};

