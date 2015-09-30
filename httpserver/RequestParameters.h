#pragma once

#include "FileIncoming.h"

#include <unordered_map>
#include <chrono>

namespace HttpServer
{
	struct request_parameters
	{
		std::unordered_map<std::string, std::string> incoming_headers;
		std::unordered_multimap<std::string, std::string> incoming_params;
		std::unordered_multimap<std::string, std::string> incoming_data;
		std::unordered_multimap<std::string, FileIncoming> incoming_files;

		std::unordered_map<std::string, std::string> outgoing_headers;

		std::string method;
		std::string version;
		std::string uri_reference;

		std::chrono::milliseconds timeout;

		size_t keep_alive_count;
		int connection_params;
		int app_exit_code;

		void clear();
	};
};