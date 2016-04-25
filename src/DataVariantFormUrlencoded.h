#pragma once

#include "DataVariantAbstract.h"

namespace HttpServer
{
	class DataVariantFormUrlencoded: public DataVariantAbstract
	{
	public:
		DataVariantFormUrlencoded();

	public:
		virtual bool parse
		(
			const SocketAdapter &sock,
			std::string & str,
			const size_t leftBytes,
			std::unordered_map<std::string, std::string> &contentParams,
			struct request_parameters &rp
		) override;
	};
};