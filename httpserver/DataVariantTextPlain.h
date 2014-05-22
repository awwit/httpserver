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
			const Socket &,
			const std::chrono::milliseconds &,
			const std::string,
			const size_t,
			const std::unordered_map<std::string, std::string> &,
			std::unordered_multimap<std::string, std::string> &,
			std::unordered_multimap<std::string, FileIncoming> &
		) override;
	};
};