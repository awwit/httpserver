#pragma once

#include "ServerProtocol.h"
#include "../../transfer/http2/Http2.h"

namespace HttpServer
{
	class ServerHttp2Protocol : public ServerProtocol
	{
	protected:
		Http2::IncStream *stream;

	protected:
		static uint8_t getPaddingSize(const size_t dataSize);

	public:
		ServerHttp2Protocol(
			Socket::Adapter &sock,
			const ServerSettings &settings,
			ServerControls &controls, Http2::IncStream *stream
		) noexcept;

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
			struct Request &rp,
			const void *src
		) const override;
	};
}
