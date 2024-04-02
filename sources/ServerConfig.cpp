#include "../includes/ServerConfig.hpp"
#include "../includes/Location.hpp"
#include "../includes/Webserv.hpp"
#include "../includes/file_utils.hpp"
#include "../includes/utils.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <map>
#include <stdexcept>

ServerConfig::ServerConfig() {
	_port = 0;
	_host = 0;
	_server_name = "";
	_root = "";
	_client_max_body_size = MAX_CONTENT_LENGTH;
	_index = "";
	_listen_fd = 0;
	_autoindex = false;
	initErrorPages();
}

ServerConfig::~ServerConfig() {}

ServerConfig::ServerConfig(const ServerConfig& source) {
	if (this == &source)
		return;

	*this = source;
}

ServerConfig& ServerConfig::operator=(const ServerConfig& source) {
	if (this == &source)
		return *this;

	_server_name = source._server_name;
	_root = source._root;
	_port = source._port;
	_host = source._host;
	_client_max_body_size = source._client_max_body_size;
	_index = source._index;
	_error_pages = source._error_pages;
	_locations = source._locations;
	_listen_fd = source._listen_fd;
	_autoindex = source._autoindex;
	_server_address = source._server_address;
	return *this;
}

void ServerConfig::initErrorPages() {
	_error_pages[301] = "";
	_error_pages[302] = "";
	_error_pages[400] = "";
	_error_pages[401] = "";
	_error_pages[402] = "";
	_error_pages[403] = "";
	_error_pages[404] = "";
	_error_pages[405] = "";
	_error_pages[406] = "";
	_error_pages[500] = "";
	_error_pages[501] = "";
	_error_pages[502] = "";
	_error_pages[503] = "";
	_error_pages[504] = "";
	_error_pages[505] = "";
}

void ServerConfig::setServerName(std::string server_name) {
	checkToken(server_name);
	_server_name = server_name;
}

void ServerConfig::setHost(std::string parameter) {
	checkToken(parameter);
	if (parameter == "localhost")
		parameter = "127.0.0.1";
	if (!isValidHost(parameter))
		throw std::runtime_error("webserv: server config: Wrong syntax: host");
	_host = inet_addr(parameter.data());
}

void ServerConfig::setRoot(std::string root) {
	checkToken(root);
	if (getTypePath(root) == FOLDER) {
		_root = root;
		return;
	}
	char dir[1024];
	getcwd(dir, 1024);
	std::string full_root = dir + root;
	if (getTypePath(full_root) != FOLDER)
		throw std::runtime_error("webserv: server config: Wrong syntax: root");
	_root = full_root;
}

void ServerConfig::setPort(std::string parameter) {
	unsigned int port;

	port = 0;
	checkToken(parameter);
	for (size_t i = 0; i < parameter.length(); i++) {
		if (!isdigit(parameter[i]))
			throw std::runtime_error(
				"webserv: server config: Wrong syntax: port");
	}
	port = ft_stoi((parameter));
	if (port < 1 || port > 65636)
		throw std::runtime_error("webserv: server config: Wrong syntax: port");
	_port = (uint16_t)port;
}

void ServerConfig::setClientMaxBodySize(std::string parameter) {
	unsigned long body_size;

	body_size = 0;
	checkToken(parameter);
	for (size_t i = 0; i < parameter.length(); i++) {
		if (parameter[i] < '0' || parameter[i] > '9')
			throw std::runtime_error(
				"webserv: server config: Wrong syntax: client_max_body_size");
	}
	if (!ft_stoi(parameter))
		throw std::runtime_error(
			"webserv: server config: Wrong syntax: client_max_body_size");
	body_size = ft_stoi(parameter);
	_client_max_body_size = body_size;
}

void ServerConfig::setIndex(std::string index) {
	checkToken(index);
	_index = index;
}

void ServerConfig::setAutoindex(std::string autoindex) {
	checkToken(autoindex);
	if (autoindex != "on" && autoindex != "off")
		throw std::runtime_error(
			"webserv: server config: Wrong syntax: autoindex");
	if (autoindex == "on")
		_autoindex = true;
}

/* checks if there is such a default error code. If there is, it overwrites the
path to the file, sourcewise it creates a new pair: error code - path to the
file */
void ServerConfig::setErrorPages(std::vector<std::string>& parameter) {
	if (parameter.empty())
		return;
	if (parameter.size() & 1)
		throw std::runtime_error(
			"webserv: server config: Error page initialization failed");
	for (size_t i = 0; i < parameter.size() - 1; i++) {
		for (size_t j = 0; j < parameter[i].size(); j++) {
			if (!isdigit(parameter[i][j]))
				throw std::runtime_error(
					"webserv: server config: Invalid error code");
		}
		if (parameter[i].size() != 3)
			throw std::runtime_error(
				"webserv: server config: Invalid error code");
		short code_error = ft_stoi(parameter[i]);
		if (statusCodeString(code_error) == "Undefined" || code_error < 400)
			throw std::runtime_error(
				"webserv: server config: Incorrect error code: " +
				parameter[i]);
		i++;
		std::string path = parameter[i];
		checkToken(path);
		if (getTypePath(path) != FOLDER) {
			if (getTypePath(_root + path) != REGULAR_FILE)
				throw std::runtime_error("webserv: server config: Incorrect "
										 "path for error page file: " +
										 _root + path);
			if (checkFile(_root + path, F_OK) == -1 ||
				checkFile(_root + path, R_OK) == -1)
				throw std::runtime_error(
					"webserv: server config: Error page file :" + _root + path +
					" is not accessible");
		}
		std::map<short, std::string>::iterator it =
			_error_pages.find(code_error);
		if (it != _error_pages.end())
			_error_pages[code_error] = path;
		else
			_error_pages.insert(make_pair(code_error, path));
	}
}

void ServerConfig::setLocation(std::string& path,
							   std::vector<std::string>& parameter) {
	Location new_location;
	std::vector<std::string> methods;
	bool flag_methods = false;
	bool flag_autoindex = false;
	bool flag_max_size = false;

	new_location.setPath(path);
	for (size_t i = 0; i < parameter.size(); i++) {
		if (parameter[i] == "root" && (i + 1) < parameter.size()) {
			if (!new_location.getRootLocation().empty())
				throw std::runtime_error(
					"webserv: server config: Root of location is duplicated");
			checkToken(parameter[++i]);
			if (getTypePath(parameter[i]) == FOLDER)
				new_location.setRootLocation(parameter[i]);
			else
				new_location.setRootLocation(_root + parameter[i]);
		} else if ((parameter[i] == "allow_methods" ||
					parameter[i] == "methods") &&
				   (i + 1) < parameter.size()) {
			if (flag_methods)
				throw std::runtime_error(
					"webserv: server config: Allow_methods of location is "
					"duplicated");
			while (++i < parameter.size()) {
				if (parameter[i].find(";") == std::string::npos) {
					new_location.enableMethod(parameter[i]);
					if (i + 1 >= parameter.size())
						throw std::runtime_error(
							"webserv: server config: Invalid token");
				} else {
					checkToken(parameter[i]);
					new_location.enableMethod(parameter[i]);
					break;
				}
			}
			flag_methods = true;
		} else if (parameter[i] == "autoindex" && (i + 1) < parameter.size()) {
			if (path == "/cgi-bin")
				throw std::runtime_error("webserv: server config: parameter "
										 "autoindex not allow for CGI");
			if (flag_autoindex)
				throw std::runtime_error("webserv: server config: Autoindex of "
										 "location is duplicated");
			checkToken(parameter[++i]);
			new_location.setAutoindex(parameter[i]);
			flag_autoindex = true;
		} else if (parameter[i] == "index" && (i + 1) < parameter.size()) {
			if (!new_location.getIndexLocation().empty())
				throw std::runtime_error(
					"webserv: server config: Index of location is duplicated");
			checkToken(parameter[++i]);
			new_location.setIndexLocation(parameter[i]);
		} else if (parameter[i] == "return" && (i + 1) < parameter.size()) {
			if (path == "/cgi-bin")
				throw std::runtime_error("webserv: server config: parameter "
										 "return not allow for CGI");
			if (!new_location.getReturn().empty())
				throw std::runtime_error(
					"webserv: server config: Return of location is duplicated");
			checkToken(parameter[++i]);
			new_location.setReturn(parameter[i]);
		} else if (parameter[i] == "alias" && (i + 1) < parameter.size()) {
			if (path == "/cgi-bin")
				throw std::runtime_error("webserv: server config: parameter "
										 "alias not allow for CGI");
			if (!new_location.getAlias().empty())
				throw std::runtime_error(
					"webserv: server config: Alias of location is duplicated");
			checkToken(parameter[++i]);
			new_location.setAlias(parameter[i]);
		} else if (parameter[i] == "cgi_ext" && (i + 1) < parameter.size()) {
			std::vector<std::string> extension;
			while (++i < parameter.size()) {
				if (parameter[i].find(";") != std::string::npos) {
					checkToken(parameter[i]);
					extension.push_back(parameter[i]);
					break;
				} else {
					extension.push_back(parameter[i]);
					if (i + 1 >= parameter.size())
						throw std::runtime_error(
							"webserv: server config: Token is invalid");
				}
			}
			new_location.setCgiExtension(extension);
		} else if (parameter[i] == "cgi_path" && (i + 1) < parameter.size()) {
			std::vector<std::string> path;
			while (++i < parameter.size()) {
				if (parameter[i].find(";") != std::string::npos) {
					checkToken(parameter[i]);
					path.push_back(parameter[i]);
					break;
				} else {
					path.push_back(parameter[i]);
					if (i + 1 >= parameter.size())
						throw std::runtime_error(
							"webserv: server config: Token is invalid");
				}
				if (parameter[i].find("/python") == std::string::npos &&
					parameter[i].find("/bash") == std::string::npos)
					throw std::runtime_error(
						"webserv: server config: cgi_path is invalid");
			}
			new_location.setCgiPath(path);
		} else if (parameter[i] == "client_max_body_size" &&
				   (i + 1) < parameter.size()) {
			if (flag_max_size)
				throw std::runtime_error("webserv: server config: Maxbody_size "
										 "of location is duplicated");
			checkToken(parameter[++i]);
			new_location.setMaxBodySize(parameter[i]);
			flag_max_size = true;
		} else if (i < parameter.size())
			throw std::runtime_error(
				"webserv: server config: parameter in a location is invalid");
	}
	if (path != "/cgi-bin" && new_location.getIndexLocation().empty())
		new_location.setIndexLocation(_index);
	if (!flag_max_size)
		new_location.setMaxBodySize(_client_max_body_size);
	checkLocation(new_location);
	if (_locations.find(path) != _locations.end())
		throw std::runtime_error("webserv: server config: duplicate location");
	_locations[path] = new_location;
}

void ServerConfig::setFd(int fd) { _listen_fd = fd; }

bool ServerConfig::isValidHost(std::string host) const {
	struct sockaddr_in sockaddr;
	return inet_pton(AF_INET, host.c_str(), &(sockaddr.sin_addr)) ? true
																  : false;
}

bool ServerConfig::isValidErrorPages() {
	std::map<short, std::string>::const_iterator it;
	for (it = _error_pages.begin(); it != _error_pages.end(); it++) {
		if (it->first < 100 || it->first > 599 ||
			checkFile(getRoot() + it->second, F_OK) < 0 ||
			checkFile(getRoot() + it->second, R_OK) < 0)
			return false;
	}
	return true;
}

void ServerConfig::checkLocation(Location& location) const {
	if (location.getPath() == "/cgi-bin") {
		if (location.getCgiPath().empty() ||
			location.getCgiExtension().empty() ||
			location.getIndexLocation().empty())
			throw std::runtime_error(
				"webserv: server config: Failed CGI validation");

		if (checkFile(location.getIndexLocation(), R_OK) < 0) {
			std::string path = location.getRootLocation() + location.getPath() +
							   "/" + location.getIndexLocation();
			if (getTypePath(path) != REGULAR_FILE) {
				std::string root = getcwd(NULL, 0);
				location.setRootLocation(root);
				path = root + location.getPath() + "/" +
					   location.getIndexLocation();
			}
			if (path.empty() || getTypePath(path) != REGULAR_FILE ||
				checkFile(path, R_OK) < 0)
				throw std::runtime_error(
					"webserv: server config: Failed CGI validation");
		}
		if (location.getCgiPath().size() != location.getCgiExtension().size())
			throw std::runtime_error(
				"webserv: server config: Failed CGI validation");
		std::vector<std::string>::const_iterator it;
		for (it = location.getCgiPath().begin();
			 it != location.getCgiPath().end(); ++it) {
			if (getTypePath(*it) == BAD)
				throw std::runtime_error(
					"webserv: server config: Failed CGI validation");
		}
		std::vector<std::string>::const_iterator it_path;
		for (it = location.getCgiExtension().begin();
			 it != location.getCgiExtension().end(); ++it) {
			std::string tmp = *it;
			if (tmp != ".py" && tmp != ".sh" && tmp != "*.py" && tmp != "*.sh")
				throw std::runtime_error(
					"webserv: server config: Failed CGI validation");
			for (it_path = location.getCgiPath().begin();
				 it_path != location.getCgiPath().end(); ++it_path) {
				std::string tmp_path = *it_path;
				if (tmp == ".py" || tmp == "*.py") {
					if (tmp_path.find("python") != std::string::npos)
						location._ext_path.insert(make_pair(".py", tmp_path));
				} else if (tmp == ".sh" || tmp == "*.sh") {
					if (tmp_path.find("bash") != std::string::npos)
						location._ext_path[".sh"] = tmp_path;
				}
			}
		}
		if (location.getCgiPath().size() != location.getExtensionPath().size())
			throw std::runtime_error(
				"webserv: server config: Failed CGI validation");
	} else {
		if (location.getPath()[0] != '/')
			throw std::runtime_error(
				"webserv: server config: Failed path in location validation");

		if (location.getRootLocation().empty())
			location.setRootLocation(_root);
		if (existsAndIsReadable(location.getRootLocation() +
									location.getPath() + "/",
								location.getIndexLocation()))
			return;
		if (!location.getReturn().empty() &&
			existsAndIsReadable(location.getRootLocation(),
								location.getReturn()))
			throw std::runtime_error("webserv: server config: Failed "
									 "redirection file in location validation");

		if (!location.getAlias().empty() &&
			existsAndIsReadable(location.getRootLocation(),
								location.getAlias()))
			throw std::runtime_error("webserv: server config: Failed alias "
									 "file in location validation");
	}
}

const std::string& ServerConfig::getServerName() { return _server_name; }

const std::string& ServerConfig::getRoot() { return _root; }

const bool& ServerConfig::getAutoindex() { return _autoindex; }

const in_addr_t& ServerConfig::getHost() { return _host; }

const uint16_t& ServerConfig::getPort() { return _port; }

const size_t& ServerConfig::getClientMaxBodySize() {
	return _client_max_body_size;
}

const std::map<std::string, Location>& ServerConfig::getLocations() {
	return _locations;
}

const std::map<short, std::string>& ServerConfig::getErrorPages() {
	return _error_pages;
}

const std::string& ServerConfig::getIndex() { return _index; }

int ServerConfig::getFd() { return _listen_fd; }

Location& ServerConfig::getLocationKey(std::string key) {
	std::map<std::string, Location>::iterator element = _locations.find(key);

	if (element == _locations.end())
		throw std::runtime_error(
			"webserv: server config: Path to location not found");
	return element->second;
}

// check is a properly end of parameter
void ServerConfig::checkToken(std::string& parameter) {
	size_t pos = parameter.rfind(';');
	if (pos != parameter.size() - 1)
		throw std::runtime_error("webserv: server config: Token is invalid");
	parameter.erase(pos);
}

// socket setup and binding
void ServerConfig::setupServer() {
	_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listen_fd == -1)
		throw std::runtime_error(std::string("webserv: socket error ") +
								 strerror(errno));

	int option_value = 1;
	setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &option_value,
			   sizeof(int));
	memset(&_server_address, 0, sizeof(_server_address));
	_server_address.sin_family = AF_INET;
	_server_address.sin_addr.s_addr = _host;
	_server_address.sin_port = htons(_port);
	if (bind(_listen_fd, (struct sockaddr*)&_server_address,
			 sizeof(_server_address)) == -1)
		throw std::runtime_error(std::string("webserv: bind error ") +
								 strerror(errno));
}
