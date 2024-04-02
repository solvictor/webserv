#pragma once

#include "Client.hpp"
#include "Response.hpp"

class ServerManager {
public:
	ServerManager(std::vector<ServerConfig>& servers);
	~ServerManager();
	ServerManager(const ServerManager& source);
	ServerManager& operator=(const ServerManager& source);

	void setupServers();
	void runServers();

private:
	std::vector<ServerConfig>& _servers;
	std::map<int, ServerConfig> _servers_map;
	std::map<int, Client> _clients_map;
	fd_set _recv_fd_pool;
	fd_set _write_fd_pool;
	int _biggest_fd;

	void acceptNewConnection(ServerConfig&);
	void checkTimeout();
	void initializeSets();
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
