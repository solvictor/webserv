#include "../includes/ServerManager.hpp"
#include "../includes/Logger.hpp"
#include "../includes/Webserv.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unistd.h>
#include <vector>

volatile sig_atomic_t ServerManager::active = true;

ServerManager::ServerManager() {}

ServerManager::~ServerManager() {
	while (!_clients.empty())
		closeConnection(_clients.begin()->first);

	for (std::map<int, ServerConfig>::iterator it = _servers.begin();
		 it != _servers.end(); ++it)
		close(it->second.getFd());
}

ServerManager::ServerManager(const ServerManager& source) {
	if (this == &source)
		return;

	*this = source;
}

ServerManager& ServerManager::operator=(const ServerManager& source) {
	if (this == &source)
		return *this;

	_servers = source._servers;
	_clients = source._clients;
	_recv_fd_pool = source._recv_fd_pool;
	_write_fd_pool = source._write_fd_pool;
	_biggest_fd = source._biggest_fd;
	return *this;
}

void ServerManager::setupServers(std::vector<ServerConfig>& servers) {
	std::cout << std::endl;
	Logger::log(LIGHT_BLUE, false, "Initializing servers...");
	char buf[INET_ADDRSTRLEN];
	bool serverDub;
	for (std::vector<ServerConfig>::iterator it = servers.begin();
		 it != servers.end(); ++it) {
		serverDub = false;
		for (std::vector<ServerConfig>::iterator it2 = servers.begin();
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
	_biggest_fd = 0;
	initializeSets(servers);
}

void ServerManager::runServers() {
	fd_set recv_set_cpy;
	fd_set write_set_cpy;
	int select_ret;

	struct timeval timer;

	while (ServerManager::active) {
		timer.tv_sec = 1;
		timer.tv_usec = 0;
		recv_set_cpy = _recv_fd_pool;
		write_set_cpy = _write_fd_pool;

		select_ret = select(_biggest_fd + 1, &recv_set_cpy, &write_set_cpy,
							NULL, &timer);
		if (select_ret < 0) {
			if (ServerManager::active)
				throw std::runtime_error(
					std::string("webserv: server manager: select() failed"));
			break;
		}

		for (int fd = 0; fd <= _biggest_fd; ++fd) {
			if (FD_ISSET(fd, &recv_set_cpy) && _servers.count(fd))
				acceptNewConnection(_servers.find(fd)->second);
			else if (FD_ISSET(fd, &recv_set_cpy) && _clients.count(fd))
				readRequest(fd);
			else if (FD_ISSET(fd, &write_set_cpy) && _clients.count(fd)) {
				CgiState cgi_state = _clients[fd].response.getCgiState();
				if (cgi_state == PROCESSING &&
					FD_ISSET(_clients[fd].response._cgi_obj.pipe_in[1],
							 &write_set_cpy))
					sendCgiBody(_clients[fd]);
				else if (cgi_state == PROCESSING &&
						 FD_ISSET(_clients[fd].response._cgi_obj.pipe_out[0],
								  &recv_set_cpy))
					readCgiResponse(_clients[fd]);
				else if ((cgi_state == NO_CGI || cgi_state == FINISHED) &&
						 FD_ISSET(fd, &write_set_cpy))
					sendResponse(fd);
			}
		}
		checkTimeout();
	}
}

/* Checks time passed for clients since last message, If more than
 * CONNECTION_TIMEOUT, close connection */
void ServerManager::checkTimeout() {
	int fd;
	for (std::map<int, Client>::iterator it = _clients.begin();
		 it != _clients.end(); ++it) {
		if (time(NULL) - it->second.getLastTime() > CONNECTION_TIMEOUT) {
			fd = it->first;
			if (closeConnection(fd))
				Logger::log(YELLOW, false, "Client %d timed out", fd);
			return;
		}
	}
}

/* initialize recv+write fd_sets and add all server listening sockets to
 * _recv_fd_pool. */
void ServerManager::initializeSets(std::vector<ServerConfig>& servers) {
	FD_ZERO(&_recv_fd_pool);
	FD_ZERO(&_write_fd_pool);

	// adds servers sockets to _recv_fd_pool set
	for (std::vector<ServerConfig>::iterator it = servers.begin();
		 it != servers.end(); ++it) {
		// Now it calls listen() twice on even if two servers have the same
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
		_servers.insert(std::make_pair(it->getFd(), *it));
	}
	// at this stage _biggest_fd will belong to the last server created.
	_biggest_fd = servers.back().getFd();
}

void ServerManager::acceptNewConnection(ServerConfig& server) {
	struct sockaddr_in client_address;
	long client_address_size = sizeof(client_address);
	int client_sock = -1;
	Client new_client(server);
	char buf[INET_ADDRSTRLEN];

	client_sock = accept(server.getFd(), (struct sockaddr*)&client_address,
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
	if (_clients.count(client_sock))
		_clients.erase(client_sock);
	_clients.insert(std::make_pair(client_sock, new_client));
}

bool ServerManager::closeConnection(const int fd) {
	Client& client = _clients[fd];
	if (client.response.getCgiState() == PROCESSING) {
		Logger::log(BLUE, true, "CGI processing on connection closing");
		Cgi& cgi = client.response._cgi_obj;

		kill(cgi.getCgiPid(), SIGTERM);
		client.response.setErrorResponse(502);
		client.response.setCgiState(FINISHED);

		int cgi_fd = cgi.pipe_in[1];
		if (cgi_fd > 2 && FD_ISSET(cgi_fd, &_write_fd_pool))
			removeFromSet(cgi_fd, _write_fd_pool);

		cgi_fd = cgi.pipe_out[0];
		if (cgi_fd > 2 && FD_ISSET(cgi_fd, &_recv_fd_pool))
			removeFromSet(cgi_fd, _recv_fd_pool);
		return false;
	}

	if (FD_ISSET(fd, &_write_fd_pool))
		removeFromSet(fd, _write_fd_pool);
	if (FD_ISSET(fd, &_recv_fd_pool))
		removeFromSet(fd, _recv_fd_pool);

	close(fd);
	_clients.erase(fd);
	return true;
}

void ServerManager::sendResponse(const int& fd) {
	Client& client = _clients[fd];
	std::string response = client.response.getRes();

	int bytes_sent = 0;
	if (response.length() < MESSAGE_BUFFER)
		bytes_sent = write(fd, response.c_str(), response.length());
	else
		bytes_sent = write(fd, response.c_str(), MESSAGE_BUFFER);

	if (bytes_sent < 0) {
		Logger::log(RED, true,
					"webserv: server manager: write() failed with fd %d", fd);
		closeConnection(fd);
	} else if (bytes_sent == 0 || (size_t)bytes_sent == response.length()) {
		Logger::log(BLUE, false, "Response sent to socket %d, Status=<%d>", fd,
					client.response.getCode());
		if (client.request.keepAlive() == false || client.request.errorCode() ||
			client.response.getCgiState()) {
			Logger::log(RED, false, "Client %d: Connection closed", fd);
			closeConnection(fd);
		} else {
			removeFromSet(fd, _write_fd_pool);
			addToSet(fd, _recv_fd_pool);
			client.clearClient();
		}
	} else {
		client.updateTime();
		client.response.cutRes(bytes_sent);
	}
}

void ServerManager::assignServer(Client& client) {
	for (std::map<int, ServerConfig>::iterator it = _servers.begin();
		 it != _servers.end(); ++it) {
		if (client.server.getHost() == it->second.getHost() &&
			client.server.getPort() == it->second.getPort() &&
			client.request.getServerName() == it->second.getServerName()) {
			client.setServer(it->second);
			return;
		}
	}
}

void ServerManager::readRequest(const int& fd) {
	char buffer[MESSAGE_BUFFER];
	int bytes_read = read(fd, buffer, MESSAGE_BUFFER);

	if (bytes_read == 0) {
		Logger::log(YELLOW, false,
					"webserv: server manager: Client %d closed connection", fd);
		closeConnection(fd);
		return;
	}
	if (bytes_read < 0) {
		Logger::log(LIGHT_RED, true,
					"webserv: server manager: read() failed with fd %d", fd);
		closeConnection(fd);
		return;
	}

	Client& client = _clients[fd];

	client.updateTime();
	client.request.feed(buffer, bytes_read);
	memset(buffer, 0, sizeof(buffer));

	if (client.request.parsingCompleted() || client.request.errorCode()) {
		assignServer(client);
		Logger::log(BLUE, false,
					"Request recived from socket %d, Method=<%s> URI=<%s>", fd,
					methodToString(client.request.getMethod()).c_str(),
					client.request.getPath().c_str());
		client.buildResponse();
		if (client.response.getCgiState()) {
			handleReqBody(client);
			addToSet(client.response._cgi_obj.pipe_in[1], _write_fd_pool);
			addToSet(client.response._cgi_obj.pipe_out[0], _recv_fd_pool);
		}
		removeFromSet(fd, _recv_fd_pool);
		addToSet(fd, _write_fd_pool);
	}
}

void ServerManager::handleReqBody(Client& client) {
	if (client.request.getBody().length() == 0) {
		std::fstream file(client.response._cgi_obj.getCgiPath().c_str());
		client.request.setBody(toString(file.rdbuf()));
	}
}

// Send request body to CGI script
void ServerManager::sendCgiBody(Client& client) {
	std::string& req_body = client.request.getBody();
	Cgi& cgi = client.response._cgi_obj;

	int bytes_sent = 0;
	if (req_body.empty())
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
		client.response.setErrorResponse(500);
	} else if (bytes_sent == 0 || (size_t)bytes_sent == req_body.length()) {
		removeFromSet(cgi.pipe_in[1], _write_fd_pool);
		close(cgi.pipe_in[1]);
		close(cgi.pipe_out[1]);
	} else {
		client.updateTime();
		req_body = req_body.substr(bytes_sent);
	}
}

// Reads CGI script's output
void ServerManager::readCgiResponse(Client& client) {
	char buffer[MESSAGE_BUFFER * 2];
	Cgi& cgi = client.response._cgi_obj;
	int bytes_read = read(cgi.pipe_out[0], buffer, MESSAGE_BUFFER * 2);

	if (bytes_read == 0) {
		removeFromSet(cgi.pipe_out[0], _recv_fd_pool);
		close(cgi.pipe_in[0]);
		close(cgi.pipe_out[0]);
		int status;
		waitpid(cgi.getCgiPid(), &status, 0);
		if (WEXITSTATUS(status))
			client.response.setErrorResponse(502);
		client.response.setCgiState(FINISHED);
		if (client.response._response_content.find("HTTP/1.1") ==
			std::string::npos)
			client.response._response_content.insert(0, "HTTP/1.1 200 OK\r\n");
		return;
	}
	if (bytes_read < 0) {
		Logger::log(LIGHT_RED, true,
					"webserv: server manager: read() failed with %d",
					cgi.pipe_out[0]);
		removeFromSet(cgi.pipe_out[0], _recv_fd_pool);
		close(cgi.pipe_in[0]);
		close(cgi.pipe_out[0]);
		client.response.setCgiState(FINISHED);
		client.response.setErrorResponse(500);
		return;
	}
	client.updateTime();
	client.response._response_content.append(buffer, bytes_read);
	memset(buffer, 0, sizeof(buffer));
}

void ServerManager::addToSet(const int fd, fd_set& set) {
	FD_SET(fd, &set);
	if (fd > _biggest_fd)
		_biggest_fd = fd;
}

void ServerManager::removeFromSet(const int fd, fd_set& set) {
	FD_CLR(fd, &set);
	if (fd == _biggest_fd)
		_biggest_fd--;
}

void stopHandler(int sig __attribute__((unused))) {
	ServerManager::active = false;
	std::cout << "\033[2D";
	Logger::log(LIGHT_GREEN, true, "Server closed");
}
