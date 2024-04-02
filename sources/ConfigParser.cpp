#include "../includes/ConfigParser.hpp"
#include "../includes/Location.hpp"
#include "../includes/ServerConfig.hpp"
#include "../includes/file_utils.hpp"
#include <iostream>
#include <map>
#include <stdexcept>
#include <unistd.h>

ConfigParser::ConfigParser() : _nb_server(0) {}

ConfigParser::~ConfigParser() {
	_servers.clear();
	_server_config.clear();
}

ConfigParser::ConfigParser(const ConfigParser& source) {
	if (this == &source)
		return;

	*this = source;
}

ConfigParser& ConfigParser::operator=(const ConfigParser& source) {
	if (this == &source)
		return *this;

	_servers = source._servers;
	_server_config = source._server_config;
	_nb_server = source._nb_server;
	return *this;
}

void ConfigParser::createCluster(const std::string& config_path) {
	std::string content;

	if (getTypePath(config_path) != REGULAR_FILE)
		throw std::runtime_error("webserv: parser: Invalid config");
	if (checkFile(config_path, R_OK) == -1)
		throw std::runtime_error("webserv: parser: Invalid config permissions");
	content = readFile(config_path);
	if (content.empty())
		throw std::runtime_error("webserv: parser: Config is empty");
	removeComments(content);
	removeWhiteSpace(content);
	splitServers(content);
	if (_server_config.size() != _nb_server)
		throw std::runtime_error("webserv: parser: Invalid number of servers");
	for (size_t i = 0; i < _nb_server; i++) {
		ServerConfig server;
		createServer(_server_config[i], server);
		_servers.push_back(server);
	}
	if (_nb_server > 1)
		checkServers();
}

void ConfigParser::removeComments(std::string& content) {
	size_t pos;

	pos = content.find('#');
	while (pos != std::string::npos) {
		size_t pos_end;
		pos_end = content.find('\n', pos);
		content.erase(pos, pos_end - pos);
		pos = content.find('#');
	}
}

void ConfigParser::removeWhiteSpace(std::string& content) {
	size_t i = 0;

	while (content[i] && isspace(content[i]))
		i++;
	content = content.substr(i);
	i = content.length() - 1;
	while (i && isspace(content[i]))
		i--;
	content = content.substr(0, i + 1);
}

void ConfigParser::splitServers(std::string& content) {
	size_t start = 0;
	size_t end = 1;

	if (content.find("server", 0) == std::string::npos)
		throw std::runtime_error("webserv: parser: Server did not find");
	while (start != end && start < content.length()) {
		start = findStartServer(start, content);
		end = findEndServer(start, content);
		if (start == end)
			throw std::runtime_error("webserv: parser: Problem with scope");
		_server_config.push_back(content.substr(start, end - start + 1));
		_nb_server++;
		start = end + 1;
	}
}

size_t ConfigParser::findStartServer(size_t start, std::string& content) {
	size_t i;

	for (i = start; content[i]; i++) {
		if (content[i] == 's')
			break;
		if (!isspace(content[i]))
			throw std::runtime_error(
				"webserv: parser: Wrong character out of server scope{}");
	}
	if (!content[i])
		return start;
	if (content.compare(i, 6, "server") != 0)
		throw std::runtime_error(
			"webserv: parser: Wrong character out of server scope{}");
	i += 6;
	while (content[i] && isspace(content[i]))
		i++;
	if (content[i] != '{')
		throw std::runtime_error(
			"webserv: parser: Wrong character out of server scope{}");
	return i;
}

size_t ConfigParser::findEndServer(size_t start, std::string& content) {
	size_t i;
	size_t scope;

	scope = 0;
	for (i = start + 1; content[i]; i++) {
		if (content[i] == '{')
			scope++;
		if (content[i] == '}') {
			if (!scope)
				return i;
			scope--;
		}
	}
	return start;
}

static std::vector<std::string> split_parameters(std::string line,
												 std::string sep) {
	std::vector<std::string> str;
	std::string::size_type start, end;

	start = end = 0;
	while (true) {
		end = line.find_first_of(sep, start);
		if (end == std::string::npos)
			break;
		str.push_back(line.substr(start, end - start));
		start = line.find_first_not_of(sep, end);
		if (start == std::string::npos)
			break;
	}
	return str;
}

void ConfigParser::createServer(std::string& config, ServerConfig& server) {
	std::vector<std::string> parameters;
	std::vector<std::string> error_codes;
	int flag_loc = 1;
	bool flag_autoindex = false;
	bool flag_max_size = false;

	parameters = split_parameters(config += ' ', std::string(" \n\t"));
	if (parameters.size() < 3)
		throw std::runtime_error("webserv: parser: Failed server validation");
	for (size_t i = 0; i < parameters.size(); i++) {
		if (parameters[i] == "listen" && (i + 1) < parameters.size() &&
			flag_loc) {
			if (server.getPort())
				throw std::runtime_error("webserv: parser: Port is duplicated");
			server.setPort(parameters[++i]);
		} else if (parameters[i] == "location" && (i + 1) < parameters.size()) {
			std::string path;
			i++;
			if (parameters[i] == "{" || parameters[i] == "}")
				throw std::runtime_error(
					"webserv: parser: Wrong character in server scope{}");
			path = parameters[i];
			std::vector<std::string> codes;
			if (parameters[++i] != "{")
				throw std::runtime_error(
					"webserv: parser: Wrong character in server scope{}");
			i++;
			while (i < parameters.size() && parameters[i] != "}")
				codes.push_back(parameters[i++]);
			server.setLocation(path, codes);
			if (i < parameters.size() && parameters[i] != "}")
				throw std::runtime_error(
					"webserv: parser: Wrong character in server scope{}");
			flag_loc = 0;
		} else if (parameters[i] == "host" && (i + 1) < parameters.size() &&
				   flag_loc) {
			if (server.getHost())
				throw std::runtime_error("webserv: parser: Host is duplicated");
			server.setHost(parameters[++i]);
		} else if (parameters[i] == "root" && (i + 1) < parameters.size() &&
				   flag_loc) {
			if (!server.getRoot().empty())
				throw std::runtime_error("webserv: parser: Root is duplicated");
			server.setRoot(parameters[++i]);
		} else if (parameters[i] == "error_page" &&
				   (i + 1) < parameters.size() && flag_loc) {
			while (++i < parameters.size()) {
				error_codes.push_back(parameters[i]);
				if (parameters[i].find(';') != std::string::npos)
					break;
				if (i + 1 >= parameters.size())
					throw std::runtime_error("webserv: parser: Wrong character "
											 "out of server scope{}");
			}
		} else if (parameters[i] == "client_max_body_size" &&
				   (i + 1) < parameters.size() && flag_loc) {
			if (flag_max_size)
				throw std::runtime_error(
					"webserv: parser: Client_max_body_size is duplicated");
			server.setClientMaxBodySize(parameters[++i]);
			flag_max_size = true;
		} else if (parameters[i] == "server_name" &&
				   (i + 1) < parameters.size() && flag_loc) {
			if (!server.getServerName().empty())
				throw std::runtime_error(
					"webserv: parser: Server_name is duplicated");
			server.setServerName(parameters[++i]);
		} else if (parameters[i] == "index" && (i + 1) < parameters.size() &&
				   flag_loc) {
			if (!server.getIndex().empty())
				throw std::runtime_error(
					"webserv: parser: Index is duplicated");
			server.setIndex(parameters[++i]);
		} else if (parameters[i] == "autoindex" &&
				   (i + 1) < parameters.size() && flag_loc) {
			if (flag_autoindex)
				throw std::runtime_error(
					"webserv: parser: Autoindex of server is duplicated");
			server.setAutoindex(parameters[++i]);
			flag_autoindex = true;
		} else if (parameters[i] != "}" && parameters[i] != "{") {
			if (!flag_loc)
				throw std::runtime_error(
					"webserv: parser: parameters after location");
			throw std::runtime_error("webserv: parser: Unsupported directive");
		}
	}
	if (server.getRoot().empty())
		server.setRoot("/;");
	if (server.getHost() == 0)
		server.setHost("localhost;");
	if (server.getIndex().empty())
		server.setIndex("index.html;");
	if (existsAndIsReadable(server.getRoot(), server.getIndex()))
		throw std::runtime_error(
			"webserv: parser: Index from config file not found or unreadable");
	if (!server.getPort())
		throw std::runtime_error("webserv: parser: Port not found");
	server.setErrorPages(error_codes);
	if (!server.isValidErrorPages())
		throw std::runtime_error("webserv: parser: Incorrect path for error "
								 "page or number of error");
}

void ConfigParser::checkServers() {
	std::vector<ServerConfig>::iterator it1;
	std::vector<ServerConfig>::iterator it2;

	for (it1 = _servers.begin(); it1 != _servers.end() - 1; it1++) {
		for (it2 = it1 + 1; it2 != _servers.end(); it2++) {
			if (it1->getPort() == it2->getPort() &&
				it1->getHost() == it2->getHost() &&
				it1->getServerName() == it2->getServerName())
				throw std::runtime_error(
					"webserv: parser: Failed server validation");
		}
	}
}

std::vector<ServerConfig>& ConfigParser::getServers() { return _servers; }

std::ostream& operator<<(std::ostream& stream, ConfigParser& configParser) {
	std::vector<ServerConfig>& servers = configParser.getServers();
	stream << "------------- Config -------------" << std::endl;
	for (size_t i = 0; i < servers.size(); i++) {
		stream << "Server #" << i + 1 << std::endl;
		stream << "Server name: " << servers[i].getServerName() << std::endl;
		stream << "Host: " << servers[i].getHost() << std::endl;
		stream << "Root: " << servers[i].getRoot() << std::endl;
		stream << "Index: " << servers[i].getIndex() << std::endl;
		stream << "Port: " << servers[i].getPort() << std::endl;
		stream << "Max BSize: " << servers[i].getClientMaxBodySize()
			   << std::endl;
		stream << "Error pages: " << servers[i].getErrorPages().size()
			   << std::endl;
		std::map<short, std::string>::const_iterator it =
			servers[i].getErrorPages().begin();
		while (it != servers[i].getErrorPages().end()) {
			stream << it->first << " - " << it->second << std::endl;
			++it;
		}
		stream << "Locations: " << servers[i].getLocations().size()
			   << std::endl;
		std::map<std::string, Location>::const_iterator itl =
			servers[i].getLocations().begin();
		while (itl != servers[i].getLocations().end()) {
			stream << "name location: " << itl->first << std::endl;
			stream << "methods: " << itl->second.getPrintMethods() << std::endl;
			stream << "index: " << itl->second.getIndexLocation() << std::endl;
			if (itl->second.getCgiPath().empty()) {
				stream << "root: " << itl->second.getRootLocation()
					   << std::endl;
				if (!itl->second.getReturn().empty())
					stream << "return: " << itl->second.getReturn()
						   << std::endl;
				if (!itl->second.getAlias().empty())
					stream << "alias: " << itl->second.getAlias() << std::endl;
			} else {
				stream << "cgi root: " << itl->second.getRootLocation()
					   << std::endl;
				stream << "sgi_path: " << itl->second.getCgiPath().size()
					   << std::endl;
				stream << "sgi_ext: " << itl->second.getCgiExtension().size()
					   << std::endl;
			}
			++itl;
		}
		itl = servers[i].getLocations().begin();
		stream << "-----------------------------" << std::endl;
	}
	return stream;
}
