#pragma once

#include "Cgi.hpp"
#include "Request.hpp"
#include "ServerConfig.hpp"

enum CgiState { NO_CGI, CGI_PROCESSING, CGI_FINISHED };

class ServerConfig;

class Response {
public:
	Response();
	Response(Request& req);
	~Response();
	Response(const Response& source);
	Response& operator=(const Response& source);

	std::string getRes();
	int getCode() const;

	void setRequest(Request& req);
	void setServer(ServerConfig& server);

	void buildResponse();
	void clear();
	void cutRes(size_t i);
	CgiState getCgiState();
	void setCgiState(CgiState state);
	void setErrorResponse(short code);

	Cgi _cgi_obj;

	std::string removeBoundary(std::string& body, std::string& boundary);
	std::string _response_content;

	Request request;

private:
	ServerConfig _server;
	std::string _target_file;
	std::vector<uint8_t> _body;
	size_t _body_length;
	std::string _response_body;
	std::string _location;
	short _code;
	CgiState _cgi; // 0->NoCGI 1->CGI write/read to/from script
				   // 2->CGI read/write done
	int _cgi_fd[2];
	size_t _cgi_response_length;
	bool _auto_index;

	bool buildBody();
	bool readFile();
	void setStatusLine();
	void setHeaders();
	void setServerDefaultErrorPages();
	void contentType();
	void contentLength();
	void connection();
	void server();
	void location();
	void date();
	int handleTarget();
	void buildErrorBody();
	bool reqError();
	int handleCgi(std::string& location_key);
	int handleCgiTemp(std::string& location_key);
};