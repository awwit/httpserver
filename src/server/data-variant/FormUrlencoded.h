#pragma once

#include "Abstract.h"

namespace DataVariant
{
	class FormUrlencoded: public Abstract
	{
	public:
		FormUrlencoded() noexcept;

	public:
		virtual bool parse(const std::string &buf, Transfer::request_data *rd, DataReceiver *dr) const override;
	};
}
