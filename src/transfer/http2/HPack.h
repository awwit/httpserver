#pragma once

#include "Http2.h"

#include <cstddef>
#include <string>
#include <vector>

namespace HPack
{
	void pack(
		std::vector<char> &dest,
		const std::vector<std::pair<std::string, std::string> > &headers,
		Http2::DynamicTable &dynamicTable
	);

	// TODO: replace IncStream to DynamicTable if possible
	bool unpack(
		const void *src,
		const size_t srcSize,
		Http2::IncStream &stream
	);
}
