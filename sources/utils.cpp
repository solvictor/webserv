#include "../includes/utils.hpp"
#include "../includes/Logger.hpp"
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>

int ft_stoi(std::string str) {
	if (str.length() > 11)
		throw std::runtime_error("ft_stoi: '" + str + "' may overflow");

	if (str.empty())
		throw std::runtime_error("ft_stoi: Invalid input, empty string");

	for (size_t i = 0; i < str.length(); ++i) {
		if (!isdigit(str[i]))
			throw std::runtime_error("ft_stoi: Invalid input '" + str + "'");
	}

	std::stringstream ss(str);

	long res;
	ss >> res;

	if (res < INT32_MIN || res > INT32_MAX)
		throw std::runtime_error("ft_stoi: '" + str + "' overflow");

	return res;
}

unsigned int hexToDec(const std::string& nb) {
	unsigned int x;

	std::stringstream ss;
	ss << nb;
	ss >> std::hex >> x;

	return x;
}

std::string statusCodeString(short statusCode) {
	switch (statusCode) {
	case 100:
		return "Continue";
	case 101:
		return "Switching Protocol";
	case 200:
		return "OK";
	case 201:
		return "Created";
	case 202:
		return "Accepted";
	case 203:
		return "Non-Authoritative Information";
	case 204:
		return "No Content";
	case 205:
		return "Reset Content";
	case 206:
		return "Partial Content";
	case 300:
		return "Multiple Choice";
	case 301:
		return "Moved Permanently";
	case 302:
		return "Moved Temporarily";
	case 303:
		return "See source";
	case 304:
		return "Not Modified";
	case 307:
		return "Temporary Redirect";
	case 308:
		return "Permanent Redirect";
	case 400:
		return "Bad Request";
	case 401:
		return "Unauthorized";
	case 403:
		return "Forbidden";
	case 404:
		return "Not Found";
	case 405:
		return "Method Not Allowed";
	case 406:
		return "Not Acceptable";
	case 407:
		return "Proxy Authentication Required";
	case 408:
		return "Request Timeout";
	case 409:
		return "Conflict";
	case 410:
		return "Gone";
	case 411:
		return "Length Required";
	case 412:
		return "Precondition Failed";
	case 413:
		return "Payload Too Large";
	case 414:
		return "URI Too Long";
	case 415:
		return "Unsupported Media Type";
	case 416:
		return "Requested Range Not Satisfiable";
	case 417:
		return "Expectation Failed";
	case 418:
		return "I'm a teapot";
	case 421:
		return "Misdirected Request";
	case 425:
		return "Too Early";
	case 426:
		return "Upgrade Required";
	case 428:
		return "Precondition Required";
	case 429:
		return "Too Many Requests";
	case 431:
		return "Request Header Fields Too Large";
	case 451:
		return "Unavailable for Legal Reasons";
	case 500:
		return "Internal Server Error";
	case 501:
		return "Not Implemented";
	case 502:
		return "Bad Gateway";
	case 503:
		return "Service Unavailable";
	case 504:
		return "Gateway Timeout";
	case 505:
		return "HTTP Version Not Supported";
	case 506:
		return "Variant Also Negotiates";
	case 507:
		return "Insufficient Storage";
	case 510:
		return "Not Extended";
	case 511:
		return "Network Authentication Required";
	default:
		return "Undefined";
	}
}

std::string getErrorPage(short statusCode) {
	return "<html>\r\n<head><title>" + toString(statusCode) + " " +
		   statusCodeString(statusCode) + " </title></head>\r\n" +
		   "<body>\r\n" + "<center><h1>" + toString(statusCode) + " " +
		   statusCodeString(statusCode) + "</h1></center>\r\n";
}

int buildHtmlIndex(std::string& dir_name, std::vector<uint8_t>& body,
				   size_t& body_len) {
	DIR* directory = opendir(dir_name.c_str());
	if (directory == NULL) {
		Logger::log(LIGHT_RED, true, "webserv: opendir() failed with name %s",
					dir_name.c_str());
		return 1;
	}

	std::string page;
	page.append("<html>\n");
	page.append("<head>\n");
	page.append("<title> Index of ");
	page.append(dir_name);
	page.append("</title>\n");
	page.append("</head>\n");
	page.append("<body >\n");
	page.append("<h1> Index of " + dir_name + "</h1>\n");
	page.append("<table style=\"width:80%; font-size: 15px\">\n");
	page.append("<hr>\n");
	page.append("<th style=\"text-align:left\"> File Name </th>\n");
	page.append("<th style=\"text-align:left\"> Last Modification  </th>\n");
	page.append("<th style=\"text-align:left\"> File Size </th>\n");

	struct stat file_stat;
	std::string file_path;

	struct dirent* entityStruct;
	while ((entityStruct = readdir(directory))) {
		if (strcmp(entityStruct->d_name, ".") == 0)
			continue;
		file_path = dir_name + entityStruct->d_name;
		stat(file_path.c_str(), &file_stat);
		page.append("<tr>\n");
		page.append("<td>\n");
		page.append("<a href=\"");
		page.append(entityStruct->d_name);
		if (S_ISDIR(file_stat.st_mode))
			page.append("/");
		page.append("\">");
		page.append(entityStruct->d_name);
		if (S_ISDIR(file_stat.st_mode))
			page.append("/");
		page.append("</a>\n");
		page.append("</td>\n");
		page.append("<td>\n");
		page.append(ctime(&file_stat.st_mtime));
		page.append("</td>\n");
		page.append("<td>\n");
		if (!S_ISDIR(file_stat.st_mode))
			page.append(toString(file_stat.st_size));
		page.append("</td>\n");
		page.append("</tr>\n");
	}
	page.append("</table>\n");
	page.append("<hr>\n");

	page.append("</body>\n");
	page.append("</html>\n");

	body.insert(body.begin(), page.begin(), page.end());
	body_len = body.size();

	if (closedir(directory))
		throw std::runtime_error("webserv: closedir() failed");
	return 0;
}

std::string methodToString(HttpMethod method) {
	switch (method) {
	case GET:
		return "GET";
	case POST:
		return "POST";
	case DELETE:
		return "DELETE";
	case PUT:
		return "PUT";
	case HEAD:
		return "HEAD";
	case NONE:
		return "NONE";
	default:
		return "Unknown";
	}
}

std::string getMimeType(std::string extension) {
	static std::map<std::string, std::string> types;

	if (types.empty()) {
		types[".html"] = "text/html";
		types[".htm"] = "text/html";
		types[".css"] = "text/css";
		types[".ico"] = "image/x-icon";
		types[".avi"] = "video/x-msvideo";
		types[".bmp"] = "image/bmp";
		types[".doc"] = "application/msword";
		types[".gif"] = "image/gif";
		types[".gz"] = "application/x-gzip";
		types[".ico"] = "image/x-icon";
		types[".jpg"] = "image/jpeg";
		types[".jpeg"] = "image/jpeg";
		types[".png"] = "image/png";
		types[".txt"] = "text/plain";
		types[".mp3"] = "audio/mp3";
		types[".pdf"] = "application/pdf";
		types["default"] = "text/html";
	}

	if (types.find(extension) == types.end())
		return types["default"];
	return types[extension];
}
