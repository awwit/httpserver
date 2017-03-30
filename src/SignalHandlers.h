#pragma once

#include "server/Server.h"

bool bindSignalHandlers(HttpServer::Server *server) noexcept;

void stopSignalHandlers() noexcept;
