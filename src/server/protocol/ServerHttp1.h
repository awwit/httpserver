#pragma once

#include "ServerProtocol.h"
#include "../../transfer/HttpStatusCode.h"

namespace HttpServer
{
	class ServerHttp1 : public ServerProtocol
	{
	private:
		const ServerApplicationSettings *getApplicationSettings(
			struct Request &rp,
			const bool isSecureConnection
		) const;

		Http::StatusCode getRequestData(
			struct Request &rp,
			std::string &str_buf,
			const ServerApplicationSettings &appSets
		) const;

	protected:
		void useHttp1Protocol(
			struct Request &rp,
			std::vector<char> &buf,
			std::string &str_buf
		) const;

	public:
		ServerHttp1(
			Socket::Adapter &sock,
			const ServerSettings &settings,
			ServerControls &controls
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
			struct Request &req,
			const void *src
		) const override;

		virtual ServerProtocol *process() override;
		virtual void close() override;
	};
}
