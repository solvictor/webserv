#pragma once

#include "Request.hpp"
#include "Response.hpp"

class Client {
public:
	Client();
	Client(ServerConfig& server);
	~Client();
	Client(const Client& source);
	Client& operator=(const Client& source);

	const time_t& getLastTime() const;

	void setSocket(int& socket);
	void setServer(ServerConfig& server);
	void buildResponse();
	void updateTime();

	void clearClient();
	Response response;
	Request request;
	ServerConfig server;

private:
	int _client_socket;
	struct sockaddr_in _client_address;
	time_t _last_msg_time;
};