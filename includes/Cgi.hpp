#pragma once

#include "Location.hpp"
#include <sys/types.h>

class Request;

class Cgi {
private:
	std::map<std::string, std::string> _env;
	std::vector<std::string> _ch_env;
	std::vector<std::string> _argv;
	std::string _cgi_path;
	pid_t _cgi_pid;
	int _exit_status;

public:
	int pipe_in[2];
	int pipe_out[2];

	Cgi();
	Cgi(std::string path);
	~Cgi();
	Cgi(const Cgi& source);
	Cgi& operator=(const Cgi& source);

	void initEnv(Request& req, Location& loc);
	void initEnvCgi(Request& req, const Location& loc);
	void execute(short& error_code);
	void clear();

	void setCgiPath(const std::string& cgi_path);

	const pid_t& getCgiPid() const;
	const std::string& getCgiPath() const;

	std::string getPathInfo(std::string& path,
							const std::vector<std::string>& extensions);
	std::string decode(std::string& path);
};