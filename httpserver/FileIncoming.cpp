
#include "FileIncoming.h"

#include <fstream>

namespace HttpServer
{
	FileIncoming::FileIncoming(const std::string &fileName, const std::string &fileType, const size_t fileSize)
		: file_name(fileName), file_type(fileType), file_size(fileSize)
	{
		
	}

	FileIncoming::FileIncoming(const FileIncoming &file)
		: file_name(file.file_name), file_type(file.file_type), file_size(file.file_size)
	{
		
	}

	FileIncoming::FileIncoming(FileIncoming &&file)
		: file_name(file.file_name), file_type(file.file_type), file_size(file.file_size)
	{
		file.file_name.clear();
		file.file_type.clear();
		file.file_size = 0;
	}

	bool FileIncoming::isExists() const
	{
		std::ifstream file(file_name, std::ifstream::binary);

		bool is_exists = file.good();

		file.close();

		return is_exists;
	}
};