
#include "ServerWebSocket.h"

namespace HttpServer
{
	ServerWebSocket::ServerWebSocket(
		Socket::Adapter &sock,
		const ServerSettings &settings,
		ServerControls &controls
	) noexcept
		: ServerProtocol(sock, settings, controls)
	{

	}

	ServerWebSocket::ServerWebSocket(const ServerProtocol &prot) noexcept
		: ServerProtocol(prot)
	{

	}

	bool ServerWebSocket::sendHeaders(
		const Http::StatusCode status,
		std::vector<std::pair<std::string, std::string> > &headers,
		const std::chrono::milliseconds &timeout,
		const bool endStream
	) const	{
		return false;
	}

	long ServerWebSocket::sendData(
		const void *src,
		size_t size,
		const std::chrono::milliseconds &timeout,
		DataTransfer *dt
	) const {
		return 0;
	}

	bool ServerWebSocket::packRequestParameters(
		std::vector<char> &buf,
		const struct Request &rp,
		const std::string &rootDir
	) const {
		return false;
	}

	void ServerWebSocket::unpackResponseParameters(
		struct Request &req,
		const void *src
	) const	{

	}

	ServerProtocol *ServerWebSocket::process() {
		return this;
	}

	void ServerWebSocket::close() {}
}
