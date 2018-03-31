#pragma once

#include "../socket/AdapterTls.h"
#include "FileIncoming.h"

#include <unordered_map>

namespace Transfer
{
	struct request_data {
		std::unordered_multimap<std::string, std::string> incoming_headers;
		std::unordered_multimap<std::string, std::string> incoming_data;
		std::unordered_multimap<std::string, Transfer::FileIncoming> incoming_files;
	};

	struct app_request {
		const System::native_socket_type socket;
		const ::gnutls_session_t tls_session;
		const void * const request_data;
	};
}
