
#include "Abstract.h"

namespace DataVariant
{
	const std::string &Abstract::getName() const noexcept {
		return this->data_variant_name;
	}

	void *Abstract::createStateStruct(
		const Transfer::request_data *rd,
		const std::unordered_map<std::string, std::string> &contentParams
	) const {
		return nullptr;
	}

	void Abstract::destroyStateStruct(void *st) const noexcept {}
}
