#pragma once

#include "ServerRequest.h"
#include "ServerResponse.h"

#include <string>
#include <functional>

namespace HttpServer
{
	struct ServerApplicationSettings
	{
		std::string root_dir;
		std::string temp_dir;
		size_t request_max_size;

		std::function<int(server_request *, server_response *)> application_call;
		std::function<bool()> application_init;
		std::function<void()> application_final;
	};
};