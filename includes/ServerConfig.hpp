#pragma once

#include <map>
#include <netinet/in.h>
#include <string>
#include <vector>

static std::string serverparameters[] = {
	"server_name", "listen",		"root",
	"index",	   "allow_methods", "client_body_buffer_size"};

class Location;

class ServerConfig {
private:
	uint16_t _port;
	in_addr_t _host;
	std::string _server_name;
	std::string _root;
	unsigned long _client_max_body_size;
	std::string _index;
	bool _autoindex;
	std::map<short, std::string> _error_pages;
	std::map<std::string, Location> _locations;
	struct sockaddr_in _server_address;
	int _listen_fd;

public:
	ServerConfig();
	~ServerConfig();
	ServerConfig(const ServerConfig& source);
	ServerConfig& operator=(const ServerConfig& source);

	void initErrorPages();

	void setServerName(std::string server_name);
	void setHost(std::string parameter);
	void setRoot(std::string root);
	void setFd(int fd);
	void setPort(std::string parameter);
	void setClientMaxBodySize(std::string parameter);
	void setErrorPages(std::vector<std::string>& parameter);
	void setIndex(std::string index);
	void setLocation(std::string& nameLocation,
					 std::vector<std::string>& parameter);
	void setAutoindex(std::string autoindex);

	bool isValidHost(std::string host) const;
	bool isValidErrorPages();
	void checkLocation(Location& location) const;

	const std::string& getServerName();
	const uint16_t& getPort();
	const in_addr_t& getHost();
	const size_t& getClientMaxBodySize();
	const std::map<std::string, Location>& getLocations();
	const std::string& getRoot();
	const std::map<short, std::string>& getErrorPages();
	const std::string& getIndex();
	const bool& getAutoindex();
	Location& getLocationKey(std::string key);

	static void checkToken(std::string& parameter);
	void setupServer();
	int getFd();
};