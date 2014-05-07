#pragma once

#include "Server.h"

#ifdef WIN32
	#include <Windows.h>
	#include <thread>

	extern std::thread threadMessageLoop;
	extern char wndClassName[];
#endif

#include <signal.h>

extern HttpServer::Server *globalServerPtr;

int bindSignalsHandles(HttpServer::Server *server);
