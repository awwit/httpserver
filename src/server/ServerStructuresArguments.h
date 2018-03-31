#pragma once

#include <string>

namespace HttpServer
{
	struct server_start_args
	{
		std::string server_name;
		std::string config_path;
		bool force;
	};
}
