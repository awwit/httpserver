
#include "FileIncoming.h"

#include <fstream>

namespace HttpServer
{
	FileIncoming::FileIncoming(const std::string &fileName, const std::string &fileType, const size_t fileSize)
	{
		file_name = fileName;
		file_type = fileType;
		file_size = fileSize;
	}

	bool FileIncoming::isExists() const
	{
		std::ifstream file(file_name);

		bool is_exists = file.good();

		file.close();

		return is_exists;
	}
};