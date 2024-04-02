#pragma once

#include <string>

#define MAX_LEN 8192
#define RESET "\033[0m"
#define BOLD "\033"
#define RED "\033[31m"
#define LIGHT_RED "\033[1;31m"
#define GREEN "\033[32m"
#define LIGHT_GREEN "\033[1;32m"
#define YELLOW "\033[33m"
#define LIGHT_YELLOW "\033[1;33m"
#define BLUE "\033[34m"
#define LIGHT_BLUE "\033[1;34m"

class Logger {

public:
	static void log(const char* color, bool error, const char* message, ...);

	static std::string currentTime();
	static void setActive(bool state);

private:
	static bool active;
};