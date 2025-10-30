#pragma once
#include <string>
#include <fstream>

class FileUtils
{
public:
	bool static WriteFile(const std::string& path, const std::string& dst = "");
	std::string static GetFile(const std::string& path);
	std::string static GetFileName(const std::string& path);
};

