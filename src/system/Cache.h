#pragma once

#include <cstdint>

namespace System
{
	constexpr int cacheLine = 64;

	template<typename T>
	struct CachePadding
	{
	private:
		static constexpr int padding_size = cacheLine - sizeof(T) % cacheLine;

		uint8_t padding[padding_size > 0 ? padding_size : sizeof(void *)];
	};

	template<std::size_t T>
	struct CachePaddingSize
	{
	private:
		uint8_t padding[cacheLine - T % cacheLine];
	};
}
