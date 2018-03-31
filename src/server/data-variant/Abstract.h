#pragma once

#include "../../socket/Adapter.h"
#include "../Request.h"

namespace DataVariant
{
	struct DataReceiver;

	class Abstract
	{
	protected:
		std::string data_variant_name;

	public:
		const std::string &getName() const noexcept;

	public:
		/**
		 * Virtual destructor
		 */
		virtual ~Abstract() noexcept = default;

		virtual void *createStateStruct(
			const Transfer::request_data *rd,
			const std::unordered_map<std::string, std::string> &contentParams
		) const;

		virtual bool parse(
			const std::string &buf,
			Transfer::request_data *rd, DataReceiver *dr
		) const = 0;

		virtual void destroyStateStruct(void *st) const noexcept;
	};

	struct DataReceiver
	{
		const Abstract *data_variant;
		void *ss;
		size_t full_size;
		size_t recv_total;
		size_t left;
		void *reserved;
	};
}
