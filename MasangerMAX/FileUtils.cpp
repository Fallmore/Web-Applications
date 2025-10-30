#include "FileUtils.h"

bool FileUtils::WriteFile(const std::string& path, const std::string& dst)
{
	std::ifstream ifs = std::ifstream(path);
	std::string line, file_name = GetFileName(path);
	std::ofstream of = std::ofstream(dst + file_name);
	if (ifs && of) {

		while (std::getline(ifs, line)) {
			of << line << "\n";
		}
	}
	else {
		return false;
	}

	ifs.close();
	of.close();
	return true;
}

std::string FileUtils::GetFile(const std::string& path)
{
	std::ifstream ifs = std::ifstream(path);
	std::string line, file_name = GetFileName(path), lines;
	if (ifs) {

		while (std::getline(ifs, line)) {
			lines += line + "\n";
		}
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
