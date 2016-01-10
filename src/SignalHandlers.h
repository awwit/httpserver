#pragma once

#include "Server.h"

bool bindSignalHandlers(HttpServer::Server *server);

void stopSignalHandlers();