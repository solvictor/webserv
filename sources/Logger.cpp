#include "../includes/Logger.hpp"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

bool Logger::active = true;

void Logger::log(const char* color, bool error, const char* message, ...) {
	char output[MAX_LEN];
	va_list args;

	if (!active)
		return;

	va_start(args, message);
	vsnprintf(output, MAX_LEN, message, args);
	va_end(args);

	std::ostream& out = error ? std::cerr : std::cout;
	out << color << currentTime() << output << RESET << std::endl;
}

std::string Logger::currentTime() {
	time_t now = time(NULL);
	struct tm tstruct = *localtime(&now);
	char buf[80];
	strftime(buf, sizeof(buf), "[%d-%m-%Y  %H:%M:%S]  ", &tstruct);
	return std::string(buf);
}

void Logger::setActive(bool s) { active = s; }
