#pragma once

#include "ServerProtocol.h"

namespace HttpServer
{
	class ServerWebSocket : public ServerProtocol
	{
	public:
		ServerWebSocket(
			Socket::Adapter &sock,
			const ServerSettings &settings,
			ServerControls &controls
		) noexcept;

		ServerWebSocket(const ServerProtocol &prot) noexcept;

		virtual bool sendHeaders(
			const Http::StatusCode status,
			std::vector<std::pair<std::string, std::string> > &headers,
			const std::chrono::milliseconds &timeout,
			const bool endStream
		) const override;

		virtual long sendData(
			const void *src,
			size_t size,
			const std::chrono::milliseconds &timeout,
			DataTransfer *dt
		) const override;

		virtual bool packRequestParameters(
			std::vector<char> &buf,
			const struct Request &rp,
			const std::string &rootDir
		) const override;

		virtual void unpackResponseParameters(
			struct Request &req,
			const void *src
		) const override;

		virtual ServerProtocol *process() override;
		virtual void close() override;
	};
}
