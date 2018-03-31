#pragma once

#include "Abstract.h"

namespace DataVariant
{
	class MultipartFormData: public Abstract
	{
	public:
		MultipartFormData() noexcept;

	public:
		virtual void *createStateStruct(
			const Transfer::request_data *rd,
			const std::unordered_map<std::string, std::string> &contentParams
		) const override;

		virtual bool parse(
			const std::string &buf,
			Transfer::request_data *rd,
			DataReceiver *dr
		) const override;

		virtual void destroyStateStruct(void *st) const noexcept override;
	};
}
