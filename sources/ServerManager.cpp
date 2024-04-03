#include "../includes/ServerManager.hpp"
#include "../includes/Logger.hpp"
#include "../includes/Webserv.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unistd.h>
#include <vector>

ServerManager::ServerManager(std::vector<ServerConfig>& servers)
	: _servers(servers) {}

ServerManager::~ServerManager() {
	while (!_clients_map.empty())
		closeConnection(_clients_map.begin()->first);

	for (std::vector<ServerConfig>::iterator it = _servers.begin();
		 it != _servers.end(); ++it)
		close(it->getFd());
}

ServerManager::ServerManager(const ServerManager& source)
	: _servers(source._servers) {
	if (this == &source)
		return;

	*this = source;
}

ServerManager& ServerManager::operator=(const ServerManager& source) {
	if (this == &source)
		return *this;

	_servers = source._servers;
	_servers_map = source._servers_map;
	_clients_map = source._clients_map;
	_recv_fd_pool = source._recv_fd_pool;
	_write_fd_pool = source._write_fd_pool;
	_biggest_fd = source._biggest_fd;
	return *this;
}

void ServerManager::setupServers() {
	std::cout << std::endl;
	Logger::log(LIGHT_BLUE, false, "Initializing servers...");
	char buf[INET_ADDRSTRLEN];
	bool serverDub;
	for (std::vector<ServerConfig>::iterator it = _servers.begin();
		 it != _servers.end(); ++it) {
		serverDub = false;
		for (std::vector<ServerConfig>::iterator it2 = _servers.begin();
			 it2 != it; ++it2) {
			if (it2->getHost() == it->getHost() &&
				it2->getPort() == it->getPort()) {
				it->setFd(it2->getFd());
				serverDub = true;
			}
		}
		if (!serverDub)
			it->setupServer();
		Logger::log(LIGHT_GREEN, false,
					"Server %s created with address [%s:%d]",
					it->getServerName().c_str(),
					inet_ntop(AF_INET, &it->getHost(), buf, INET_ADDRSTRLEN),
					it->getPort());
	}
}

void ServerManager::runServers() {
	fd_set recv_set_cpy;
	fd_set write_set_cpy;
	int select_ret;

	_biggest_fd = 0;
	initializeSets();
	struct timeval timer;

	while (true) {
		timer.tv_sec = 1;
		timer.tv_usec = 0;
		recv_set_cpy = _recv_fd_pool;
		write_set_cpy = _write_fd_pool;

		select_ret = select(_biggest_fd + 1, &recv_set_cpy, &write_set_cpy,
							NULL, &timer);
		if (select_ret < 0) {
			Logger::log(RESET, true, "strerror: %s", strerror(errno));
			throw std::runtime_error(
				std::string("webserv: server manager: server interruped"));
		}

		for (int fd = 0; fd <= _biggest_fd; ++fd) {
			if (FD_ISSET(fd, &recv_set_cpy) && _servers_map.count(fd))
				acceptNewConnection(_servers_map.find(fd)->second);
			else if (FD_ISSET(fd, &recv_set_cpy) && _clients_map.count(fd))
				readRequest(fd, _clients_map[fd]);
			else if (FD_ISSET(fd, &write_set_cpy) && _clients_map.count(fd)) {
				int cgi_state = _clients_map[fd].response.getCgiState();
				if (cgi_state == 1 &&
					FD_ISSET(_clients_map[fd].response._cgi_obj.pipe_in[1],
							 &write_set_cpy))
					sendCgiBody(_clients_map[fd],
								_clients_map[fd].response._cgi_obj);
				else if (cgi_state == 1 &&
						 FD_ISSET(
							 _clients_map[fd].response._cgi_obj.pipe_out[0],
							 &recv_set_cpy))
					readCgiResponse(_clients_map[fd],
									_clients_map[fd].response._cgi_obj);
				else if ((cgi_state == 0 || cgi_state == 2) &&
						 FD_ISSET(fd, &write_set_cpy))
					sendResponse(fd, _clients_map[fd]);
			}
		}
		checkTimeout();
	}
}

/* Checks time passed for clients since last message, If more than
 * CONNECTION_TIMEOUT, close connection */
void ServerManager::checkTimeout() {
	for (std::map<int, Client>::iterator it = _clients_map.begin();
		 it != _clients_map.end(); ++it) {
		if (time(NULL) - it->second.getLastTime() > CONNECTION_TIMEOUT) {
			Logger::log(YELLOW, false, "Client %d timed out", it->first);
			closeConnection(it->first);
			return;
		}
	}
}

/* initialize recv+write fd_sets and add all server listening sockets to
 * _recv_fd_pool. */
void ServerManager::initializeSets() {
	FD_ZERO(&_recv_fd_pool);
	FD_ZERO(&_write_fd_pool);

	// adds servers sockets to _recv_fd_pool set
	for (std::vector<ServerConfig>::iterator it = _servers.begin();
		 it != _servers.end(); ++it) {
		// Now it calles listen() twice on even if two servers have the same
		// host:port
		if (listen(it->getFd(), 512) == -1)
			throw std::runtime_error(
				std::string("webserv: server manager: listen error ") +
				strerror(errno));

		if (fcntl(it->getFd(), F_SETFL, O_NONBLOCK) < 0)
			throw std::runtime_error(
				std::string("webserv: server manager: fcntl error ") +
				strerror(errno));

		addToSet(it->getFd(), _recv_fd_pool);
		_servers_map.insert(std::make_pair(it->getFd(), *it));
	}
	// at this stage _biggest_fd will belong to the last server created.
	_biggest_fd = _servers.back().getFd();
}

void ServerManager::acceptNewConnection(ServerConfig& serv) {
	struct sockaddr_in client_address;
	long client_address_size = sizeof(client_address);
	int client_sock = -1;
	Client new_client(serv);
	char buf[INET_ADDRSTRLEN];

	client_sock = accept(serv.getFd(), (struct sockaddr*)&client_address,
						 (socklen_t*)&client_address_size);
	if (client_sock == -1) {
		Logger::log(LIGHT_RED, true, "webserv: server manager: accept error %s",
					strerror(errno));
		return;
	}
	Logger::log(GREEN, false, "New connection from %s, assigned socket %d",
				inet_ntop(AF_INET, &client_address, buf, INET_ADDRSTRLEN),
				client_sock);

	addToSet(client_sock, _recv_fd_pool);

	if (fcntl(client_sock, F_SETFL, O_NONBLOCK) < 0) {
		Logger::log(LIGHT_RED, true, "webserv: server manager: fcntl error %s",
					strerror(errno));
		removeFromSet(client_sock, _recv_fd_pool);
		close(client_sock);
		return;
	}
	new_client.setSocket(client_sock);
	if (_clients_map.count(client_sock))
		_clients_map.erase(client_sock);
	_clients_map.insert(std::make_pair(client_sock, new_client));
}

void ServerManager::closeConnection(const int i) {
	if (FD_ISSET(i, &_write_fd_pool))
		removeFromSet(i, _write_fd_pool);
	if (FD_ISSET(i, &_recv_fd_pool))
		removeFromSet(i, _recv_fd_pool);
	close(i);
	_clients_map.erase(i);
}

void ServerManager::sendResponse(const int& i, Client& c) {
	int bytes_sent = 0;
	std::string response = c.response.getRes();
	if (response.length() < MESSAGE_BUFFER)
		bytes_sent = write(i, response.c_str(), response.length());
	else
		bytes_sent = write(i, response.c_str(), MESSAGE_BUFFER);

	if (bytes_sent < 0) {
		Logger::log(RED, true,
					"webserv: server manager: write() failed with fd %d", i);
		closeConnection(i);
	} else if (bytes_sent == 0 || (size_t)bytes_sent == response.length()) {
		Logger::log(BLUE, false, "Response sent to socket %d, Stats=<%d>", i,
					c.response.getCode());
		if (c.request.keepAlive() == false || c.request.errorCode() ||
			c.response.getCgiState()) {
			Logger::log(RED, false, "Client %d: Connection closed", i);
			closeConnection(i);
		} else {
			removeFromSet(i, _write_fd_pool);
			addToSet(i, _recv_fd_pool);
			c.clearClient();
		}
	} else {
		c.updateTime();
		c.response.cutRes(bytes_sent);
	}
}

void ServerManager::assignServer(Client& c) {
	for (std::vector<ServerConfig>::iterator it = _servers.begin();
		 it != _servers.end(); ++it) {
		if (c.server.getHost() == it->getHost() &&
			c.server.getPort() == it->getPort() &&
			c.request.getServerName() == it->getServerName()) {
			c.setServer(*it);
			return;
		}
	}
}

void ServerManager::readRequest(const int& i, Client& c) {
	char buffer[MESSAGE_BUFFER];
	int bytes_read = 0;
	bytes_read = read(i, buffer, MESSAGE_BUFFER);
	if (bytes_read == 0) {
		Logger::log(YELLOW, false,
					"webserv: server manager: Client %d closed connection", i);
		closeConnection(i);
		return;
	}
	if (bytes_read < 0) {
		Logger::log(LIGHT_RED, true,
					"webserv: server manager: read() failed with fd %d", i);
		closeConnection(i);
		return;
	}
	c.updateTime();
	c.request.feed(buffer, bytes_read);
	memset(buffer, 0, sizeof(buffer));

	if (c.request.parsingCompleted() || c.request.errorCode()) {
		assignServer(c);
		Logger::log(BLUE, false,
					"Request recived from socket %d, Method=<%s> URI=<%s>", i,
					methodToString(c.request.getMethod()).c_str(),
					c.request.getPath().c_str());
		c.buildResponse();
		if (c.response.getCgiState()) {
			handleReqBody(c);
			addToSet(c.response._cgi_obj.pipe_in[1], _write_fd_pool);
			addToSet(c.response._cgi_obj.pipe_out[0], _recv_fd_pool);
		}
		removeFromSet(i, _recv_fd_pool);
		addToSet(i, _write_fd_pool);
	}
}

void ServerManager::handleReqBody(Client& c) {
	if (c.request.getBody().length() == 0) {
		std::string tmp;
		std::fstream file;
		(c.response._cgi_obj.getCgiPath().c_str());
		tmp = toString(file.rdbuf());
		c.request.setBody(tmp);
	}
}

// Send request body to CGI script
void ServerManager::sendCgiBody(Client& c, Cgi& cgi) {
	int bytes_sent = 0;
	std::string& req_body = c.request.getBody();

	if (req_body.length() == 0)
		bytes_sent = 0;
	else if (req_body.length() < MESSAGE_BUFFER)
		bytes_sent = write(cgi.pipe_in[1], req_body.c_str(), req_body.length());
	else
		bytes_sent = write(cgi.pipe_in[1], req_body.c_str(), MESSAGE_BUFFER);

	if (bytes_sent < 0) {
		Logger::log(LIGHT_RED, true,
					"webserv: server manager: write() failed with fd %d",
					cgi.pipe_in[1]);
		removeFromSet(cgi.pipe_in[1], _write_fd_pool);
		close(cgi.pipe_in[1]);
		close(cgi.pipe_out[1]);
		c.response.setErrorResponse(500);
	} else if (bytes_sent == 0 || (size_t)bytes_sent == req_body.length()) {
		removeFromSet(cgi.pipe_in[1], _write_fd_pool);
		close(cgi.pipe_in[1]);
		close(cgi.pipe_out[1]);
	} else {
		c.updateTime();
		req_body = req_body.substr(bytes_sent);
	}
}

// Reads CGI script's output
void ServerManager::readCgiResponse(Client& c, Cgi& cgi) {
	char buffer[MESSAGE_BUFFER * 2];
	int bytes_read = 0;
	bytes_read = read(cgi.pipe_out[0], buffer, MESSAGE_BUFFER * 2);

	if (bytes_read == 0) {
		removeFromSet(cgi.pipe_out[0], _recv_fd_pool);
		close(cgi.pipe_in[0]);
		close(cgi.pipe_out[0]);
		int status;
		waitpid(cgi.getCgiPid(), &status, 0);
		if (WEXITSTATUS(status))
			c.response.setErrorResponse(502);
		c.response.setCgiState(2);
		if (c.response._response_content.find("HTTP/1.1") == std::string::npos)
			c.response._response_content.insert(0, "HTTP/1.1 200 OK\r\n");
		return;
	}
	if (bytes_read < 0) {
		Logger::log(LIGHT_RED, true,
					"webserv: server manager: read() failed with %d",
					cgi.pipe_out[0]);
		removeFromSet(cgi.pipe_out[0], _recv_fd_pool);
		close(cgi.pipe_in[0]);
		close(cgi.pipe_out[0]);
		c.response.setCgiState(2);
		c.response.setErrorResponse(500);
		return;
	}
	c.updateTime();
	c.response._response_content.append(buffer, bytes_read);
	memset(buffer, 0, sizeof(buffer));
}

void ServerManager::addToSet(const int i, fd_set& set) {
	FD_SET(i, &set);
	if (i > _biggest_fd)
		_biggest_fd = i;
}

void ServerManager::removeFromSet(const int i, fd_set& set) {
	FD_CLR(i, &set);
	if (i == _biggest_fd)
		_biggest_fd--;
}

void stopHandler(int sig __attribute__((unused))) { std::cout << "\033[2D"; }
