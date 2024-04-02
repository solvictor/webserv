#include "../includes/Client.hpp"
#include <ctime>

Client::Client() : _last_msg_time(time(NULL)) {}

Client::~Client() {}

Client::Client(const Client& source) {
	if (this == &source)
		return;

	*this = source;
}

Client& Client::operator=(const Client& source) {
	if (this == &source)
		return *this;

	_client_socket = source._client_socket;
	_client_address = source._client_address;
	request = source.request;
	response = source.response;
	server = source.server;
	_last_msg_time = source._last_msg_time;
	return *this;
}

Client::Client(ServerConfig& server) {
	setServer(server);
	request.setMaxBodySize(server.getClientMaxBodySize());
	_last_msg_time = time(NULL);
}

void Client::setSocket(int& socket) { _client_socket = socket; }

void Client::setServer(ServerConfig& server) { response.setServer(server); }

const time_t& Client::getLastTime() const { return _last_msg_time; }

void Client::buildResponse() {
	response.setRequest(request);
	response.buildResponse();
}

void Client::updateTime() { _last_msg_time = time(NULL); }

void Client::clearClient() {
	response.clear();
	request.clear();
}