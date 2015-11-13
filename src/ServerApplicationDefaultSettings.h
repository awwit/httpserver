#pragma once

#include <string>

namespace HttpServer
{
	struct ServerApplicationDefaultSettings
	{
		std::string temp_dir;
		size_t request_max_size;
	};
};