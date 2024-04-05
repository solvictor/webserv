#pragma once

#include "Client.hpp"
#include "Response.hpp"

class ServerManager {
public:
	ServerManager();
	~ServerManager();
	ServerManager(const ServerManager& source);
	ServerManager& operator=(const ServerManager& source);

	void setupServers(std::vector<ServerConfig>& servers);
	void runServers();

private:
	std::map<int, ServerConfig> _servers;
	std::map<int, Client> _clients;
	fd_set _recv_fd_pool;
	fd_set _write_fd_pool;
	int _biggest_fd;

	void acceptNewConnection(ServerConfig&);
	void checkTimeout();
	void initializeSets(std::vector<ServerConfig>& servers);
	void readRequest(const int&, Client&);
	void handleReqBody(Client&);
	void sendResponse(const int&, Client&);
	void sendCgiBody(Client&, Cgi&);
	void readCgiResponse(Client&, Cgi&);
	void closeConnection(const int);
	void assignServer(Client&);
	void addToSet(const int, fd_set&);
	void removeFromSet(const int, fd_set&);
};

void stopHandler(int sig);
