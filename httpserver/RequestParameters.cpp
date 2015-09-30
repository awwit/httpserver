
#include "RequestParameters.h"

namespace HttpServer
{
	void request_parameters::clear()
	{
		incoming_headers.clear();
		incoming_params.clear();
		incoming_data.clear();
		incoming_files.clear();

		outgoing_headers.clear();

		method.clear();
		version.clear();
		uri_reference.clear();

		timeout = std::chrono::milliseconds::zero();
	}
};