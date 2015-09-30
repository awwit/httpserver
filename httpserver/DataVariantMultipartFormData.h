#pragma once

#include "DataVariantAbstract.h"

namespace HttpServer
{
	class DataVariantMultipartFormData: public DataVariantAbstract
	{
	public:
		DataVariantMultipartFormData();

	protected:
		bool append
		(
			const Socket &,
			const std::chrono::milliseconds &,
			std::vector<std::string::value_type> &,
			std::string &,
			const std::string &,
			const size_t &,
			size_t &,
			size_t &
		) const;

	public:
		virtual bool parse
		(
			const Socket &,
			std::string &,
			const size_t,
			std::unordered_map<std::string, std::string> &contentParams,
			struct request_parameters &rp
		) override;
	};
};