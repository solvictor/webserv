#include "../includes/Response.hpp"
#include "../includes/Logger.hpp"
#include "../includes/file_utils.hpp"
#include "../includes/utils.hpp"
#include <csignal>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

Response::Response() {
	clear();
	_cgi_fd[0] = -1;
	_cgi_fd[1] = -1;
}

Response::~Response() {
	if (_cgi_obj.getCgiPid() > 2 && !WIFEXITED(_cgi_obj.getCgiPid()))
		kill(_cgi_obj.getCgiPid(), SIGTERM);

	if (_cgi_fd[0] > 2)
		close(_cgi_fd[0]);

	if (_cgi_fd[1] > 2)
		close(_cgi_fd[1]);
}

Response::Response(Request& req) : request(req) {
	clear();
	_cgi_fd[0] = -1;
	_cgi_fd[1] = -1;
}

Response::Response(const Response& source) {
	if (this == &source)
		return;

	*this = source;
}

Response& Response::operator=(const Response& source) {
	if (this == &source)
		return *this;

	_server = source._server;
	_target_file = source._target_file;
	_body = source._body;
	_body_length = source._body_length;
	_response_body = source._response_body;
	_location = source._location;
	_code = source._code;
	_cgi = source._cgi;
	_cgi_fd[0] = source._cgi_fd[0];
	_cgi_fd[1] = source._cgi_fd[1];
	_cgi_response_length = source._cgi_response_length;
	_auto_index = source._auto_index;
	return *this;
}

void Response::contentType() {
	_response_content.append("Content-Type: ");
	if (_target_file.rfind(".", std::string::npos) != std::string::npos &&
		_code == 200)
		_response_content.append(getMimeType(
			_target_file.substr(_target_file.rfind(".", std::string::npos))));
	else
		_response_content.append(getMimeType("default"));
	_response_content.append("\r\n");
}

void Response::contentLength() {
	_response_content.append("Content-Length: ");
	_response_content.append(toString(_response_body.length()));
	_response_content.append("\r\n");
}

void Response::connection() {
	if (request.getHeader("connection") == "keep-alive")
		_response_content.append("Connection: keep-alive\r\n");
}

void Response::server() { _response_content.append("Server: webserv\r\n"); }

void Response::location() {
	if (_location.length())
		_response_content.append("Location: " + _location + "\r\n");
}

void Response::date() {
	char date[1000];
	time_t now = time(NULL);
	struct tm tm = *gmtime(&now);
	strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S %Z", &tm);
	_response_content.append("Date: ");
	_response_content.append(date);
	_response_content.append("\r\n");
}

void Response::setHeaders() {
	contentType();
	contentLength();
	connection();
	server();
	location();
	date();

	_response_content.append("\r\n");
}

static bool isAllowedMethod(HttpMethod& method, const Location& location,
							short& code) {
	if (location.getMethods() & (1 << method))
		return false;
	code = 405;
	return true;
}

static bool checkReturn(Location& loc, short& code, std::string& location) {
	if (loc.getReturn().empty())
		return false;
	code = 301;
	location = loc.getReturn();
	if (location[0] != '/')
		location.insert(location.begin(), '/');
	return true;
}

static std::string combinePaths(std::string p1, std::string p2,
								std::string p3) {
	size_t len1 = p1.length();
	size_t len2 = p2.length();

	if (p1[len1 - 1] == '/' && (!p2.empty() && p2[0] == '/'))
		p2.erase(0, 1);
	if (p1[len1 - 1] != '/' && (!p2.empty() && p2[0] != '/'))
		p1.insert(p1.end(), '/');
	if (p2[len2 - 1] == '/' && (!p3.empty() && p3[0] == '/'))
		p3.erase(0, 1);
	if (p2[len2 - 1] != '/' && (!p3.empty() && p3[0] != '/'))
		p2.insert(p1.end(), '/');
	return p1 + p2 + p3;
}

static void replaceAlias(Location& location, Request& request,
						 std::string& target_file) {
	target_file =
		combinePaths(location.getAlias(),
					 request.getPath().substr(location.getPath().length()), "");
}

static void appendRoot(Location& location, Request& request,
					   std::string& target_file) {
	target_file =
		combinePaths(location.getRootLocation(), request.getPath(), "");
}

int Response::handleCgiTemp(std::string& location_key) {
	std::string path;
	path = _target_file;
	_cgi_obj.clear();
	_cgi_obj.setCgiPath(path);
	if (pipe(_cgi_fd) < 0) {
		_code = 500;
		return 1;
	}
	_cgi = PROCESSING;
	_cgi_obj.initEnvCgi(request, _server.getLocationKey(location_key)); // + URI
	_cgi_obj.execute(_code);
	return 0;
}

/* check a file for CGI (the extension is supported, the file exists and is
 * executable) and run the CGI */
int Response::handleCgi(std::string& location_key) {
	std::string path;
	std::string exten;
	size_t pos;
	Location& target = _server.getLocationKey(location_key);

	path = request.getPath();
	if (path[0] && path[0] == '/')
		path.erase(0, 1);
	if (path == "cgi-bin")
		path += "/" + target.getIndexLocation();
	else if (path == "cgi-bin/")
		path.append(target.getIndexLocation());

	pos = path.find(".");
	if (pos == std::string::npos) {
		_code = 501;
		return 1;
	}
	exten = path.substr(pos);
	if (exten != ".py" && exten != ".sh") {
		_code = 501;
		return 1;
	}
	if (getTypePath(path) != REGULAR_FILE) {
		_code = 404;
		return 1;
	}
	if (checkFile(path, X_OK) == -1 || checkFile(path, W_OK | X_OK) == -1) {
		_code = 403;
		return 1;
	}
	if (isAllowedMethod(request.getMethod(), target, _code))
		return 1;

	_cgi_obj.clear();
	_cgi_obj.setCgiPath(path);
	if (pipe(_cgi_fd) < 0) {
		_code = 500;
		return 1;
	}

	_cgi = PROCESSING;
	_cgi_obj.initEnv(request, target); // + URI
	_cgi_obj.execute(_code);
	return 0;
}

static void getLocationMatch(std::string& path,
							 const std::map<std::string, Location>& locations,
							 std::string& location_key) {
	size_t biggest_match = 0;

	for (std::map<std::string, Location>::const_iterator it = locations.begin();
		 it != locations.end(); ++it) {
		if (path.find(it->first))
			continue;

		if (it->first == "/" || path.length() == it->first.length() ||
			path[it->first.length()] == '/') {
			if (it->first.length() > biggest_match) {
				biggest_match = it->first.length();
				location_key = it->first;
			}
		}
	}
}

int Response::handleTarget() {
	std::string location_key;

	getLocationMatch(request.getPath(), _server.getLocations(), location_key);
	if (location_key.length()) {
		Location& target_location = _server.getLocationKey(location_key);

		if (isAllowedMethod(request.getMethod(), target_location, _code)) {
			Logger::log(RED, true, "Method not allowed");
			return 1;
		}

		if (request.getBody().length() > target_location.getMaxBodySize()) {
			_code = 413;
			return 1;
		}

		if (checkReturn(target_location, _code, _location))
			return 1;

		if (target_location.getPath().find("cgi-bin") != std::string::npos)
			return handleCgi(location_key);

		if (!target_location.getAlias().empty())
			replaceAlias(target_location, request, _target_file);
		else
			appendRoot(target_location, request, _target_file);

		if (!target_location.getCgiExtension().empty() &&
			_target_file.rfind(target_location.getCgiExtension()[0]) !=
				std::string::npos)
			return handleCgiTemp(location_key);

		if (isDirectory(_target_file)) {
			if (_target_file[_target_file.length() - 1] != '/') {
				_code = 301;
				_location = request.getPath() + "/";
				return 1;
			}
			if (!target_location.getIndexLocation().empty())
				_target_file += target_location.getIndexLocation();
			else
				_target_file += _server.getIndex();
			if (!fileExists(_target_file)) {
				if (target_location.getAutoindex()) {
					_target_file.erase(_target_file.find_last_of('/') + 1);
					_auto_index = true;
					return 0;
				} else {
					_code = 403;
					return 1;
				}
			}
			if (isDirectory(_target_file)) {
				_code = 301;
				if (!target_location.getIndexLocation().empty())
					_location =
						combinePaths(request.getPath(),
									 target_location.getIndexLocation(), "");
				else
					_location =
						combinePaths(request.getPath(), _server.getIndex(), "");
				if (_location[_location.length() - 1] != '/')
					_location.insert(_location.end(), '/');

				return 1;
			}
		}
	} else {
		_target_file = combinePaths(_server.getRoot(), request.getPath(), "");
		if (isDirectory(_target_file)) {
			if (_target_file[_target_file.length() - 1] != '/') {
				_code = 301;
				_location = request.getPath() + "/";
				return 1;
			}
			_target_file += _server.getIndex();
			if (!fileExists(_target_file)) {
				_code = 403;
				return 1;
			}
			if (isDirectory(_target_file)) {
				_code = 301;
				_location =
					combinePaths(request.getPath(), _server.getIndex(), "");
				if (_location[_location.length() - 1] != '/')
					_location.insert(_location.end(), '/');
				return 1;
			}
		}
	}
	return 0;
}

bool Response::reqError() {
	if (request.errorCode()) {
		_code = request.errorCode();
		return true;
	}
	return false;
}

void Response::buildErrorBody() {
	if (!_server.getErrorPages().count(_code) ||
		_server.getErrorPages().at(_code).empty() ||
		request.getMethod() == DELETE || request.getMethod() == POST)
		setServerDefaultErrorPages();
	else {
		if (_code >= 400 && _code < 500) {
			_location = _server.getErrorPages().at(_code);
			if (_location[0] != '/')
				_location.insert(_location.begin(), '/');
		}

		_target_file = _server.getRoot() + _server.getErrorPages().at(_code);
		short old_code = _code;
		if (!readFile()) {
			_code = old_code;
			_response_body = getErrorPage(_code);
		}
	}
}

void Response::buildResponse() {
	if (reqError() || !buildBody())
		buildErrorBody();
	if (_cgi != NO_CGI)
		return;
	if (_auto_index) {
		if (buildHtmlIndex(_target_file, _body, _body_length)) {
			_code = 500;
			buildErrorBody();
		} else
			_code = 200;
		_response_body.insert(_response_body.begin(), _body.begin(),
							  _body.end());
	}
	setStatusLine();
	setHeaders();
	if (request.getMethod() != HEAD &&
		(request.getMethod() == GET || _code != 200))
		_response_content.append(_response_body);
}

bool Response::buildBody() {
	if (request.getBody().length() > _server.getClientMaxBodySize()) {
		_code = 413;
		return false;
	}
	if (handleTarget())
		return false;
	if (_cgi != NO_CGI || _auto_index || (_code && _code != 200))
		return true;

	if ((request.getMethod() == GET || request.getMethod() == HEAD) &&
		!readFile()) {
		return false;
	} else if (request.getMethod() == POST || request.getMethod() == PUT) {
		if (fileExists(_target_file) && request.getMethod() == POST) {
			_code = 204;
			return true;
		}
		std::ofstream file(_target_file.c_str(), std::ios::binary);
		if (file.fail()) {
			_code = 404;
			return false;
		}

		if (request.getMultiformFlag()) {
			std::string body = request.getBody();
			body = removeBoundary(body, request.getBoundary());
			file.write(body.c_str(), body.length());
		} else
			file.write(request.getBody().c_str(), request.getBody().length());
	} else if (request.getMethod() == DELETE) {
		if (!fileExists(_target_file)) {
			_code = 404;
			return false;
		}
		if (remove(_target_file.c_str()) != 0) {
			_code = 500;
			return false;
		}
	}
	_code = 200;
	return true;
}

bool Response::readFile() {
	std::ifstream file(_target_file.c_str());

	if (file.fail()) {
		_code = 404;
		return false;
	}
	std::ostringstream ss;
	ss << file.rdbuf();
	_response_body = ss.str();
	return true;
}

// Returns the entire reponse (Headers + Body)
std::string Response::getRes() { return _response_content; }

short Response::getCode() const { return _code; }

CgiState Response::getCgiState() { return _cgi; }

void Response::setServer(ServerConfig& server) { _server = server; }

void Response::setRequest(Request& req) { request = req; }

void Response::setCgiState(CgiState state) { _cgi = state; }

void Response::setErrorResponse(short code) {
	_response_content = "";
	_code = code;
	_response_body = "";
	buildErrorBody();
	setStatusLine();
	setHeaders();
	_response_content.append(_response_body);
}

// Constructs Status line based on status code
void Response::setStatusLine() {
	_response_content.append("HTTP/1.1 " + toString(_code) + " ");
	_response_content.append(statusCodeString(_code));
	_response_content.append("\r\n");
}

void Response::setServerDefaultErrorPages() {
	_response_body = getErrorPage(_code);
}

void Response::cutRes(size_t i) {
	_response_content = _response_content.substr(i);
}

void Response::clear() {
	_target_file.clear();
	_body.clear();
	_body_length = 0;
	_response_content.clear();
	_response_body.clear();
	_code = 200;
	_cgi = NO_CGI;
	_cgi_response_length = 0;
	_auto_index = false;
}

std::string Response::removeBoundary(std::string& body, std::string& boundary) {
	std::string buffer;
	std::string new_body;
	std::string filename;
	bool is_boundary = false;
	bool is_content = false;

	if (body.find("--" + boundary) != std::string::npos &&
		body.find("--" + boundary + "--") != std::string::npos) {
		for (size_t i = 0; i < body.size(); i++) {
			buffer.clear();
			while (body[i] != '\n') {
				buffer += body[i];
				i++;
			}
			if (!buffer.compare(("--" + boundary + "--\r"))) {
				is_content = true;
				is_boundary = false;
			}
			if (!buffer.compare(("--" + boundary + "\r")))
				is_boundary = true;
			if (is_boundary) {
				if (!buffer.compare(0, 31, "Content-Disposition: form-data;")) {
					size_t start = buffer.find("filename=\"");
					if (start != std::string::npos) {
						size_t end = buffer.find("\"", start + 10);
						if (end != std::string::npos)
							filename = buffer.substr(start + 10, end);
					}
				} else if (!buffer.compare(0, 1, "\r") && !filename.empty()) {
					is_boundary = false;
					is_content = true;
				}
			} else if (is_content) {
				if (!buffer.compare(("--" + boundary + "\r")))
					is_boundary = true;
				else if (!buffer.compare(("--" + boundary + "--\r"))) {
					new_body.erase(new_body.end() - 1);
					break;
				} else
					new_body += (buffer + "\n");
			}
		}
	}

	body.clear();
	return new_body;
}
