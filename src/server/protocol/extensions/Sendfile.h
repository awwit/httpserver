#pragma once

#include "../ServerProtocol.h"
#include "../../Request.h"

namespace HttpServer
{
	class Sendfile
	{
	private:
		static int transferFilePart(
			const ServerProtocol &prot,
			const struct Request &rp,
			const std::string &fileName,
			const std::unordered_map<std::string, std::string> &mimesTypes,
			const time_t fileTime,
			const size_t fileSize,
			const std::string &rangeHeader,
			std::vector<std::pair<std::string, std::string> > &additionalHeaders,
			const bool headersOnly
		) noexcept;

		static int transferFile(
			const ServerProtocol &prot,
			const struct Request &req,
			const std::string &fileName,
			const std::unordered_map<std::string, std::string> &mimesTypes,
			std::vector<std::pair<std::string, std::string> > &additionalHeaders,
			const bool headersOnly
		) noexcept;

	public:
		static bool isConnectionReuse(const struct Request &req) noexcept;

		static void xSendfile(
			const ServerProtocol &prot,
			struct Request &req,
			const std::unordered_map<std::string, std::string> &mimesTypes
		) noexcept;
	};
}
