#include "../includes/Cgi.hpp"
#include "../includes/Logger.hpp"
#include "../includes/Request.hpp"
#include "../includes/StringVector.hpp"
#include "../includes/utils.hpp"
#include <algorithm>
#include <cstring>
#include <unistd.h>

Cgi::Cgi() {
	_cgi_pid = -1;
	_exit_status = 0;
	_cgi_path = "";
	pipe_in[0] = -1;
	pipe_in[1] = -1;
	pipe_out[0] = -1;
	pipe_out[1] = -1;
}

Cgi::Cgi(std::string path) {
	_cgi_pid = -1;
	_exit_status = 0;
	_cgi_path = path;
	pipe_in[0] = -1;
	pipe_in[1] = -1;
	pipe_out[0] = -1;
	pipe_out[1] = -1;
}

Cgi::~Cgi() {
	_ch_env.clear();
	_argv.clear();
	_env.clear();

	if (pipe_in[0] > 2)
		close(pipe_in[0]);

	if (pipe_in[1] > 2)
		close(pipe_in[1]);

	if (pipe_out[0] > 2)
		close(pipe_out[0]);

	if (pipe_out[1] > 2)
		close(pipe_out[1]);
}

Cgi::Cgi(const Cgi& source) {
	if (this == &source)
		return;

	*this = source;
}

Cgi& Cgi::operator=(const Cgi& source) {
	if (this == &source)
		return *this;

	_env = source._env;
	_ch_env = source._ch_env;
	_argv = source._argv;
	_cgi_path = source._cgi_path;
	_cgi_pid = source._cgi_pid;
	_exit_status = source._exit_status;
	return *this;
}

const pid_t& Cgi::getCgiPid() const { return _cgi_pid; }

const std::string& Cgi::getCgiPath() const { return _cgi_path; }

void Cgi::setCgiPath(const std::string& cgi_path) { _cgi_path = cgi_path; }

void Cgi::initEnvCgi(Request& req, const Location& loc) {
	std::string cgi_exec = ("cgi-bin/" + loc.getCgiPath()[0]).c_str();
	char* cwd = getcwd(NULL, 0);

	if (_cgi_path[0] != '/') {
		std::string tmp(cwd);
		tmp.append("/");
		if (_cgi_path.length())
			_cgi_path.insert(0, tmp);
	}
	if (req.getMethod() == POST) {
		_env["CONTENT_LENGTH"] = toString(req.getBody().length());
		_env["CONTENT_TYPE"] = req.getHeader("content-type");
	}

	_env["GATEWAY_INTERFACE"] = "CGI/1.1";
	_env["SCRIPT_NAME"] = cgi_exec;
	_env["SCRIPT_FILENAME"] = _cgi_path;
	_env["PATH_INFO"] = _cgi_path;
	_env["PATH_TRANSLATED"] = _cgi_path;
	_env["REQUEST_URI"] = _cgi_path;
	_env["SERVER_NAME"] = req.getHeader("host");
	_env["SERVER_PORT"] = "8080";
	_env["REQUEST_METHOD"] = methodToString(req.getMethod());
	_env["SERVER_PROTOCOL"] = "HTTP/1.1";
	_env["REDIRECT_STATUS"] = "200";
	_env["SERVER_SOFTWARE"] = "webserv";

	std::map<std::string, std::string> request_headers = req.getHeaders();
	for (std::map<std::string, std::string>::iterator it =
			 request_headers.begin();
		 it != request_headers.end(); ++it) {
		std::string name = it->first;
		transform(name.begin(), name.end(), name.begin(), toupper);
		std::string key = "HTTP_" + name;
		_env[key] = it->second;
	}
	_ch_env.reserve(_env.size() + 1);
	std::map<std::string, std::string>::const_iterator it = _env.begin();
	for (; it != _env.end(); it++)
		_ch_env.push_back(it->first + "=" + it->second);

	_argv.push_back(cgi_exec);
	_argv.push_back(_cgi_path);
}

void Cgi::initEnv(Request& req, Location& loc) {
	size_t pos;
	std::string extension;
	std::string ext_path;

	extension = _cgi_path.substr(_cgi_path.find("."));
	std::map<std::string, std::string>::iterator it_path =
		loc._ext_path.find(extension);
	if (it_path == loc._ext_path.end())
		return;
	ext_path = loc._ext_path[extension];

	_env["AUTH_TYPE"] = "Basic";
	_env["CONTENT_LENGTH"] = req.getHeader("content-length");
	_env["CONTENT_TYPE"] = req.getHeader("content-type");
	_env["GATEWAY_INTERFACE"] = "CGI/1.1";
	pos = _cgi_path.find("cgi-bin/");
	_env["SCRIPT_NAME"] = _cgi_path;
	_env["SCRIPT_FILENAME"] =
		((pos == std::string::npos || (pos + 8) > _cgi_path.size())
			 ? ""
			 : _cgi_path.substr(pos + 8, _cgi_path.size()));
	_env["PATH_INFO"] = getPathInfo(req.getPath(), loc.getCgiExtension());
	_env["PATH_TRANSLATED"] =
		loc.getRootLocation() +
		(_env["PATH_INFO"] == "" ? "/" : _env["PATH_INFO"]);
	_env["QUERY_STRING"] = decode(req.getQuery());
	_env["REMOTE_ADDR"] = req.getHeader("host");
	pos = req.getHeader("host").find(":");
	_env["SERVER_NAME"] =
		(pos == std::string::npos ? "" : req.getHeader("host").substr(0, pos));
	_env["SERVER_PORT"] =
		(pos == std::string::npos ? ""
								  : req.getHeader("host").substr(
										pos + 1, req.getHeader("host").size()));
	_env["REQUEST_METHOD"] = methodToString(req.getMethod());
	_env["HTTP_COOKIE"] = req.getHeader("cookie");
	_env["DOCUMENT_ROOT"] = loc.getRootLocation();
	_env["REQUEST_URI"] = req.getPath() + req.getQuery();
	_env["SERVER_PROTOCOL"] = "HTTP/1.1";
	_env["REDIRECT_STATUS"] = "200";
	_env["SERVER_SOFTWARE"] = "webserv";

	_ch_env.reserve(_env.size() + 1);
	for (std::map<std::string, std::string>::const_iterator it = _env.begin();
		 it != _env.end(); it++)
		_ch_env.push_back(it->first + "=" + it->second);

	_argv.push_back(ext_path);
	_argv.push_back(_cgi_path);
}

void Cgi::execute(short& error_code) {
	if (_argv.size() < 2) {
		error_code = 500;
		return;
	}
	if (pipe(pipe_in) < 0) {
		Logger::log(LIGHT_RED, true, "webserv: cgi handler: pipe() failed");

		error_code = 500;
		return;
	}
	if (pipe(pipe_out) < 0) {
		Logger::log(LIGHT_RED, true, "webserv: cgi handler: pipe() failed");

		close(pipe_in[0]);
		close(pipe_in[1]);
		error_code = 500;
		return;
	}
	_cgi_pid = fork();
	if (_cgi_pid == -1) {
		Logger::log(LIGHT_RED, true, "webserv: cgi handler: fork() failed");
		error_code = 500;
	} else if (_cgi_pid == 0) {
		dup2(pipe_in[0], STDIN_FILENO);
		dup2(pipe_out[1], STDOUT_FILENO);
		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);
		StringVector envp(_ch_env);
		StringVector argvp(_argv);
		_exit_status = execve(argvp[0], static_cast<char**>(argvp),
							  static_cast<char**>(envp));
		exit(_exit_status);
	}
}

// Translation of parameters for QUERY_STRING environment variable
std::string Cgi::decode(std::string& path) {
	size_t token = path.find("%");
	while (token != std::string::npos) {
		if (path.length() < token + 2)
			break;

		char decimal = static_cast<char>(
			strtol(path.substr(token + 1, 2).c_str(), NULL, 16));

		path.replace(token, 3, toString(decimal));
		token = path.find("%");
	}
	return path;
}

// Isolation PATH_INFO environment variable
std::string Cgi::getPathInfo(std::string& path,
							 const std::vector<std::string>& extensions) {
	std::string tmp;
	size_t start, end;

	for (std::vector<std::string>::const_iterator it_ext = extensions.begin();
		 it_ext != extensions.end(); it_ext++) {
		start = path.find(*it_ext);
		if (start != std::string::npos)
			break;
	}
	if (start == std::string::npos)
		return "";
	if (start + 3 >= path.size())
		return "";
	tmp = path.substr(start + 3, path.size());
	if (!tmp[0] || tmp[0] != '/')
		return "";
	end = tmp.find("?");
	return end == std::string::npos ? tmp : tmp.substr(0, end);
}

void Cgi::clear() {
	_cgi_pid = -1;
	_exit_status = 0;
	_cgi_path = "";
	_env.clear();
}