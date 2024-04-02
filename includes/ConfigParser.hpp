#pragma once

#include <string>
#include <vector>

class ServerConfig;

class ConfigParser {
private:
	std::vector<ServerConfig> _servers;
	std::vector<std::string> _server_config;
	size_t _nb_server;

public:
	ConfigParser();
	~ConfigParser();
	ConfigParser(const ConfigParser& source);
	ConfigParser& operator=(const ConfigParser& source);

	void createCluster(const std::string& config_path);

	void splitServers(std::string& content);
	void removeComments(std::string& content);
	void removeWhiteSpace(std::string& content);
	size_t findStartServer(size_t start, std::string& content);
	size_t findEndServer(size_t start, std::string& content);
	void createServer(std::string& config, ServerConfig& server);
	void checkServers();
	std::vector<ServerConfig>& getServers();
};

std::ostream& operator<<(std::ostream& stream, ConfigParser& configParser);