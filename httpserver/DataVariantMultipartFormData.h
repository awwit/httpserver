#pragma once

#include "DataVariantAbstract.h"

namespace HttpServer
{
	class DataVariantMultipartFormData: public DataVariantAbstract
	{
	public:
		DataVariantMultipartFormData();

	private:
		bool append
		(
			const Socket *,
			std::vector<std::string::value_type> &,
			std::string &, const std::string &,
			const size_t &,
			size_t &,
			size_t &
		) const;

	public:
		virtual bool parse
		(
			const Socket *,
			const std::string,
			const size_t,
			const std::unordered_map<std::string, std::string> &,
			std::unordered_multimap<std::string, std::string> &,
			std::unordered_multimap<std::string, FileIncoming> &
		) override;
	};
};
