#include "Main.h"
#include "SignalsHandles.h"

#include <iostream>
#include <cstring>
#include <fstream>
#include <unordered_map>

int main(const int argc, const char *argv[])
{
	const std::unordered_map<std::string, std::function<int(HttpServer::Server &)> > commands {
		{"--help", std::mem_fn(&HttpServer::Server::help)},
		{"--start", std::mem_fn(&HttpServer::Server::start)},
		{"--restart", std::mem_fn(&HttpServer::Server::restart)},
		{"--kill", std::mem_fn(&HttpServer::Server::terminate)}
	};

	int exitcode = EXIT_FAILURE;

	if (2 == argc)
	{
		auto command = commands.find(argv[1]);

		if (commands.end() != command)
		{
			HttpServer::Server server;

			if (bindSignalsHandles(&server) )
			{
				exitcode = command->second(server);

			#ifdef WIN32
				System::sendSignal(GetCurrentProcessId(), SIGINT);
				threadMessageLoop.join();
			#endif
			}
		}
		else
		{
			std::cout << "Unknown command" << std::endl;
		}
	}
	else if (1 == argc)
	{
		std::cout << "Try: " << argv[0] << " --help" << std::endl;
	}
	else
	{
		std::cout << "Arguments failure" << std::endl;
	}

	return exitcode;
}
