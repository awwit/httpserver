#pragma once

#include "../transfer/AppRequest.h"
#include "../transfer/ProtocolVariant.h"

#include <unordered_map>
#include <chrono>

namespace HttpServer
{
	enum ConnectionParams {
		CONNECTION_CLOSE = 0,
		CONNECTION_REUSE = 1,
		CONNECTION_LEAVE_OPEN = 2
	};

	struct Request : public Transfer::request_data
	{
		std::unordered_map<std::string, std::string> outgoing_headers;

		std::string host;
		std::string path;
		std::string method;

		std::chrono::milliseconds timeout;

		size_t keep_alive_count;
		int connection_params;
		int app_exit_code;

		Transfer::ProtocolVariant protocol_variant;
	//	void *protocol_data;

	public:
		void clear() noexcept;
	};

	struct DataTransfer {
		size_t full_size;
		size_t send_total;
	};
}
