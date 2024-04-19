#include "../includes/file_utils.hpp"
#include "../includes/utils.hpp"
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

PathType getTypePath(const std::string& path) {
	struct stat buffer;

	if (stat(path.c_str(), &buffer))
		return BAD;
	if (buffer.st_mode & S_IFREG)
		return REGULAR_FILE;
	if (buffer.st_mode & S_IFDIR)
		return FOLDER;
	return OTHER;
}

int checkFile(const std::string& path, int mode) {
	return access(path.c_str(), mode);
}

bool existsAndIsReadable(const std::string& path, const std::string& index) {
	if (getTypePath(index) == REGULAR_FILE && checkFile(index, R_OK) == 0)
		return false;
	if (getTypePath(path + index) == REGULAR_FILE &&
		checkFile(path + index, R_OK) == 0)
		return false;
	return true;
}

std::string readFile(const std::string& path) {
	if (path.empty() || path.length() == 0)
		return "";

	std::ifstream config_file(path.c_str());
	if (!config_file || !config_file.is_open())
		return "";

	return toString(config_file.rdbuf());
}

bool fileExists(const std::string& f) {
	std::ifstream file(f.c_str());
	return file.good();
}

bool isDirectory(std::string path) {
	struct stat file_stat;
	if (stat(path.c_str(), &file_stat) != 0)
		return false;

	return S_ISDIR(file_stat.st_mode);
}