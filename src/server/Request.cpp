
#include "Request.h"

namespace HttpServer
{
	void Request::clear() noexcept
	{
		incoming_headers.clear();
		incoming_data.clear();
		incoming_files.clear();

		outgoing_headers.clear();

		host.clear();
		path.clear();
		method.clear();

	//	timeout = std::chrono::milliseconds::zero();
	}
}
