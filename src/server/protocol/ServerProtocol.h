#pragma once

#include "../Request.h"
#include "../ServerControls.h"
#include "../ServerSettings.h"

#include "../../transfer/HttpStatusCode.h"

namespace HttpServer
{
	class ServerProtocol
	{
	protected:
		Socket::Adapter &sock;
		const ServerSettings &settings;
		ServerControls &controls;

	public:
		ServerProtocol(
			Socket::Adapter &sock,
			const ServerSettings &settings,
			ServerControls &controls
		) noexcept;

		ServerProtocol(const ServerProtocol &prot) noexcept;
		virtual ~ServerProtocol() noexcept = default;

		static DataVariant::DataReceiver *
		createDataReceiver(
			const Transfer::request_data *rd,
			const std::unordered_map<std::string, DataVariant::Abstract *> &variants
		);

		static void destroyDataReceiver(void *src);

	protected:
		void runApplication(
			struct Request &req,
			const ServerApplicationSettings &appSets
		) const;

	public:
		virtual bool sendHeaders(
			const Http::StatusCode status,
			std::vector<std::pair<std::string, std::string> > &headers,
			const std::chrono::milliseconds &timeout,
			const bool endStream = true
		) const = 0;

		virtual long sendData(
			const void *src,
			size_t size,
			const std::chrono::milliseconds &timeout,
			DataTransfer *dt
		) const = 0;

		virtual bool packRequestParameters(
			std::vector<char> &buf,
			const struct Request &req,
			const std::string &rootDir
		) const = 0;

		virtual void unpackResponseParameters(
			struct Request &req,
			const void *src
		) const = 0;

		virtual ServerProtocol *process() = 0;
		virtual void close() = 0;
	};
}
