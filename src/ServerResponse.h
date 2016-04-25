#pragma once

#include "RawData.h"
#include "SocketAdapter.h"

#include <map>
#include <string>

namespace HttpServer
{
	struct server_response
	{
		System::native_socket_type socket;
		size_t headers_count;
		Utils::raw_pair *headers;
	};

	struct ServerResponse
	{
		SocketAdapter &socket;
		std::map<std::string, std::string> headers;
	};
};
