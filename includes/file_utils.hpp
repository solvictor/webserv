#pragma once

#include <string>

enum PathType { REGULAR_FILE, FOLDER, OTHER, BAD };

PathType getTypePath(const std::string& path);
int checkFile(const std::string& path, int mode);
bool existsAndIsReadable(const std::string& path,
									const std::string& index);
std::string readFile(const std::string& path);