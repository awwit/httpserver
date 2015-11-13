#pragma once

#include <cstddef>

namespace Utils
{
	struct raw_pair
	{
		char *key;
		char *value;
	};

	struct raw_fileinfo
	{
		char *key;
		char *file_name;
		char *file_type;
		size_t file_size;
	};
};