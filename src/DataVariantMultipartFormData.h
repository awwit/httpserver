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
			const Socket &sock,
			const std::chrono::milliseconds &timeout,
			std::vector<char> &buf,
			std::string &str_buf,
			const std::string &data_end,
			const size_t &leftBytes,
			size_t &recv_len,
			size_t &read_len
		) const;

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