#include "../includes/ConfigParser.hpp"
#include "../includes/Logger.hpp"
#include "../includes/ServerManager.hpp"
#include <cerrno>
#include <csignal>
#include <cstring>
#include <exception>
#include <stdexcept>

int main(int argc, char** argv) {
	// Logger::setActive(false); // To disable logging
	if (argc < 1 || argc > 2) {
		Logger::log(LIGHT_RED, true, "webserv: wrong arguments");
		return 1;
	}

	try {
		ConfigParser cluster;
		ServerManager master;

		if (signal(SIGPIPE, SIG_IGN) == SIG_ERR ||
			signal(SIGINT, stopHandler) == SIG_ERR)
			throw std::runtime_error(std::string("webserv: signal: ") +
									 strerror(errno));

		cluster.createCluster(argc == 1 ? "configs/default.conf" : argv[1]);
		// std::cout << cluster; // Debug

		master.setupServers(cluster.getServers());
		master.runServers();

	} catch (const std::exception& e) {
		Logger::log(LIGHT_RED, true, e.what());
		return 1;
	}
	return 0;
}
