#pragma once

#include "Server.h"

#ifdef WIN32
	#include <thread>
	std::thread threadMessageLoop;
	char myWndClassName[] = "WndClassNameConstant";
#endif

HttpServer::Server *globalServerPtr = nullptr;
