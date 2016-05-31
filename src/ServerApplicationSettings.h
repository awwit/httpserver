#pragma once

#include "ServerRequest.h"
#include "ServerResponse.h"
#include "RawData.h"

#include <unordered_set>
#include <string>
#include <functional>

namespace HttpServer
{
	struct ServerApplicationSettings
	{
		std::unordered_set<int> ports;
		std::unordered_set<int> tls_ports;

		std::string root_dir;
		std::string temp_dir;
		size_t request_max_size;

		size_t module_index;
		std::string server_module;
		std::string server_module_update;

		std::string cert_file;
		std::string key_file;
		std::string chain_file;
		std::string crl_file;
		std::string stapling_file;
		std::string dh_file;

		std::function<int(server_request *, server_response *)> application_call;
		std::function<void(Utils::raw_pair [], const size_t)> application_clear;
		std::function<bool()> application_init;
		std::function<void()> application_final;
	};
};