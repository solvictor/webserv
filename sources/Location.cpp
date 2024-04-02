#include "../includes/Location.hpp"
#include "../includes/ServerConfig.hpp"
#include "../includes/Webserv.hpp"
#include "../includes/file_utils.hpp"
#include "../includes/utils.hpp"
#include <stdexcept>

Location::Location() {
	_path = "";
	_root = "";
	_autoindex = false;
	_index = "";
	_return = "";
	_alias = "";
	_client_max_body_size = MAX_CONTENT_LENGTH;
	_methods = 0;
}

Location::Location(const Location& source) {
	if (this == &source)
		return;

	*this = source;
}

Location& Location::operator=(const Location& source) {
	if (this == &source)
		return *this;

	_path = source._path;
	_root = source._root;
	_autoindex = source._autoindex;
	_index = source._index;
	_cgi_path = source._cgi_path;
	_cgi_ext = source._cgi_ext;
	_return = source._return;
	_alias = source._alias;
	_methods = source._methods;
	_ext_path = source._ext_path;
	_client_max_body_size = source._client_max_body_size;
	return *this;
}

Location::~Location() {}

void Location::setPath(std::string& parameter) { _path = parameter; }

void Location::setRootLocation(std::string parameter) {
	if (getTypePath(parameter) != FOLDER)
		throw std::runtime_error("webserv: location: root of location");
	_root = parameter;
}

void Location::setAutoindex(std::string& parameter) {
	if (parameter == "on")
		_autoindex = true;
	else if (parameter == "off")
		_autoindex = false;
	else
		throw std::runtime_error("webserv: location: Wrong autoindex");
}

void Location::setIndexLocation(std::string& parameter) { _index = parameter; }

void Location::setReturn(std::string& parameter) { _return = parameter; }

void Location::setAlias(std::string& parameter) { _alias = parameter; }

void Location::setCgiPath(std::vector<std::string>& path) { _cgi_path = path; }

void Location::setCgiExtension(std::vector<std::string>& extension) {
	_cgi_ext = extension;
}

void Location::setMaxBodySize(std::string& parameter) {
	unsigned long body_size = 0;

	for (size_t i = 0; i < parameter.length(); i++) {
		if (!isdigit(parameter[i]))
			throw std::runtime_error(
				"webserv: location: Wrong syntax: client_max_body_size");
	}
	if (!ft_stoi(parameter))
		throw std::runtime_error(
			"webserv: location: Wrong syntax: client_max_body_size");
	body_size = ft_stoi(parameter);
	_client_max_body_size = body_size;
}

void Location::setMaxBodySize(unsigned long& parameter) {
	_client_max_body_size = parameter;
}

const std::string& Location::getPath() const { return _path; }

const std::string& Location::getRootLocation() const { return _root; }

const std::string& Location::getIndexLocation() const { return _index; }

const short& Location::getMethods() const { return _methods; }

const std::vector<std::string>& Location::getCgiPath() const {
	return _cgi_path;
}

const std::vector<std::string>& Location::getCgiExtension() const {
	return _cgi_ext;
}

const bool& Location::getAutoindex() const { return _autoindex; }

const std::string& Location::getReturn() const { return _return; }

const std::string& Location::getAlias() const { return _alias; }

const std::map<std::string, std::string>& Location::getExtensionPath() const {
	return _ext_path;
}

const unsigned long& Location::getMaxBodySize() const {
	return _client_max_body_size;
}

void Location::enableMethod(std::string& method) {
	if (method == "GET")
		_methods |= 1 << GET;
	else if (method == "POST")
		_methods |= 1 << POST;
	else if (method == "DELETE")
		_methods |= 1 << DELETE;
	else if (method == "PUT")
		_methods |= 1 << PUT;
	else if (method == "HEAD")
		_methods |= 1 << HEAD;
	else
		throw std::runtime_error(
			"webserv: location: Allow method not supported " + method);
}

std::string Location::getPrintMethods() const {
	std::ostringstream oss;
	bool first = true;

	for (int i = 0; i < 5; ++i) {
		if (_methods & (1 << i)) {
			if (!first)
				oss << ", ";
			oss << methodToString(static_cast<HttpMethod>(i));
			first = false;
		}
	}
	return oss.str();
}
