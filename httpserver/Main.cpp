#include "Main.h"
#include "SignalsHandles.h"

#include <iostream>
#include <cstring>
#include <fstream>
#include <unordered_map>

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

	if (1 < argc)
	{
		auto command = commands.find(argv[1]);

		if (commands.end() != command)
		{
			HttpServer::Server server;

			if (bindSignalsHandles(&server) )
			{
				exitcode = command->second(&server, argc, argv);

			#ifdef WIN32
				System::sendSignal(GetCurrentProcessId(), SIGINT);
				threadMessageLoop.join();
			#endif
			}
		}
		else
		{
			std::cout << "Unknown command, see --help" << std::endl;
		}
	}
	else if (1 == argc)
	{
		std::cout << "Try: " << argv[0] << " --help" << std::endl;
	}
	else
	{
		std::cout << "Arguments failure, see --help command" << std::endl;
	}

	return exitcode;
}