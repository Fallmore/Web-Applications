#include "Logger.h"

std::string Logger::LogTime() {
	std::time_t t = std::time(0);   // Get the time now
	std::tm now;
	localtime_s(&now, &t);

	return "[" + std::to_string(now.tm_hour) + ":" + std::to_string(now.tm_min) + ":" + std::to_string(now.tm_sec) + "]	";
}

void Logger::Log(std::string message) {
	std::cout << LogTime() << message << std::endl;
}