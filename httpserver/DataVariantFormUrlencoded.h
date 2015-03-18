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
			const Socket &,
			const std::chrono::milliseconds &,
			std::string &,
			const size_t,
			const std::unordered_map<std::string, std::string> &,
			std::unordered_multimap<std::string, std::string> &,
			std::unordered_multimap<std::string, FileIncoming> &
		) override;
	};
};