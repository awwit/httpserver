#pragma once

#include "DataVariantAbstract.h"

namespace HttpServer
{
	class DataVariantTextPlain: public DataVariantAbstract
	{
	public:
		DataVariantTextPlain();

	public:
		virtual bool parse
		(
			const Socket &sock,
			std::string &str,
			const size_t leftBytes,
			std::unordered_map<std::string, std::string> &contentParams,
			struct request_parameters &rp
		) override;
	};
};