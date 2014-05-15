#pragma once

#include "Server.h"

#ifdef WIN32
	#include <thread>
	std::thread threadMessageLoop;
	char wndClassName[] = "WndClassNameConstant";
#endif

HttpServer::Server *globalServerPtr = nullptr;
