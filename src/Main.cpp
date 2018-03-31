
#include "Main.h"
#include "server/Server.h"
#include "SignalHandlers.h"

#include <iostream>

int main(const int argc, const char *argv[])
{
	const std::unordered_map<std::string, std::function<int(HttpServer::Server *, const int, const char *[])> > commands {
		{"--help", std::mem_fn(&HttpServer::Server::command_help)},
		{"--start", std::mem_fn(&HttpServer::Server::command_start)},
		{"--restart", std::mem_fn(&HttpServer::Server::command_restart)},
		{"--kill", std::mem_fn(&HttpServer::Server::command_terminate)},
		{"--update-module", std::mem_fn(&HttpServer::Server::command_update_module)}
	};

	int exitcode = EXIT_FAILURE;

	if (1 < argc) {
		auto const command = commands.find(argv[1]);

		if (commands.cend() != command) {
			HttpServer::Server server;

			if (bindSignalHandlers(&server) ) {
				exitcode = command->second(&server, argc, argv);
				stopSignalHandlers();
			}
		} else {
			std::cout << "Unknown parameter, see --help" << std::endl;
		}
	}
	else if (1 == argc) {
		std::cout << "Try to run with the parameter --help" << std::endl;
	} else {
		std::cout << "The number of arguments cannot be equal to zero. Enter the parameter --help" << std::endl;
	}

	return exitcode;
}
