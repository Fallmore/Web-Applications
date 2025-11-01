#include "FileUtils.h"

bool FileUtils::WriteFile(const std::string& path, const std::string& dst)
{
	std::ifstream ifs = std::ifstream(path);
	std::string line, file_name = GetFileName(path);
	std::ofstream of = std::ofstream(dst + file_name);
	
	if (ifs && of) {
		line = { std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>() };
			of << line;
	}
	else {
		return false;
	}

	ifs.close();
	of.close();
	return true;
}

bool FileUtils::WriteFileContent(const std::string& path, const std::string& content)
{
	std::ofstream of = std::ofstream(path);
	if (of) {

		of << content;
	}
	else {
		return false;
	}

	of.close();
	return true;
}

std::string FileUtils::GetFile(const std::string& path)
{
	std::ifstream ifs = std::ifstream(path);
	std::string lines;
	if (ifs) {
		lines = { std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>() };
	}
	else {
		return "Не удалось прочитать файл(";
	}

	ifs.close();
	return lines;
}

std::string FileUtils::GetFileName(const std::string& path)
{
	return path.substr(path.find_last_of("\\") + 1);
}
