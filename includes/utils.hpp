#pragma once

#include <cstring>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

enum HttpMethod { GET, POST, DELETE, PUT, HEAD, NONE };

template <typename T>
std::string toString(const T val) {
	std::stringstream stream;
	stream << val;
	return stream.str();
}

std::string statusCodeString(short statusCode);
std::string getErrorPage(short statusCode);
int buildHtmlIndex(std::string& dir_name, std::vector<uint8_t>& body,
				   size_t& body_len);
int ft_stoi(std::string str);
std::string getMimeType(std::string extension);
std::string methodToString(HttpMethod method);
