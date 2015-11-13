#pragma once

#include "Server.h"
#include "System.h"

#ifdef WIN32
	#include <Windows.h>
	#include <thread>

	extern std::thread threadMessageLoop;
	extern char myWndClassName[];
#endif

int bindSignalsHandles(HttpServer::Server *server);
