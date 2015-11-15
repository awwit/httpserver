
#include "FileIncoming.h"

#include <fstream>

namespace HttpServer
{
	FileIncoming::FileIncoming(const std::string &fileName, const std::string &fileType, const size_t fileSize)
		: file_name(fileName), file_type(fileType), file_size(fileSize)
	{
		
	}

	FileIncoming::FileIncoming(const FileIncoming &obj)
		: file_name(obj.file_name), file_type(obj.file_type), file_size(obj.file_size)
	{
		
	}

	FileIncoming::FileIncoming(FileIncoming &&obj)
		: file_name(std::move(obj.file_name) ), file_type(std::move(obj.file_type) ), file_size(obj.file_size)
	{
		obj.file_size = 0;
	}

	bool FileIncoming::isExists() const
	{
		std::ifstream file(file_name, std::ifstream::binary);

		const bool is_exists = file.good();

		file.close();

		return is_exists;
	}
};