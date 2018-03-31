
#include "FileIncoming.h"
#include "../utils/Utils.h"

#include <fstream>

namespace Transfer
{
	FileIncoming::FileIncoming(
		std::string &&fileTmpName,
		std::string &&fileName,
		std::string &&fileType,
		const size_t fileSize
	) noexcept
		: file_tmp_name(std::move(fileTmpName) ),
		  file_name(std::move(fileName) ),
		  file_type(std::move(fileType) ),
		  file_size(fileSize)
	{

	}

	FileIncoming::FileIncoming(const FileIncoming &obj)
		: file_tmp_name(obj.file_tmp_name),
		  file_name(obj.file_name),
		  file_type(obj.file_type),
		  file_size(obj.file_size)
	{

	}

	FileIncoming::FileIncoming(FileIncoming &&obj) noexcept
		: file_tmp_name(std::move(obj.file_tmp_name) ),
		  file_name(std::move(obj.file_name) ),
		  file_type(std::move(obj.file_type) ),
		  file_size(obj.file_size)
	{
		obj.file_size = 0;
	}

	const std::string &FileIncoming::getTmpName() const noexcept {
		return this->file_tmp_name;
	}

	const std::string &FileIncoming::getName() const noexcept {
		return this->file_name;
	}

	const std::string &FileIncoming::getType() const noexcept {
		return this->file_type;
	}

	size_t FileIncoming::getSize() const noexcept {
		return this->file_size;
	}

	bool FileIncoming::isExists() const noexcept {
		std::ifstream file(this->file_tmp_name, std::ifstream::binary);
		const bool is_exists = file.good();
		file.close();
		return is_exists;
	}
}

namespace Utils
{
	void packFilesIncoming(
		std::vector<char> &buf,
		const std::unordered_multimap<std::string, Transfer::FileIncoming> &map
	) {
		packNumber(buf, map.size() );

		for (auto it = map.cbegin(); map.cend() != it; ++it) {
			packString(buf, it->first);

			const Transfer::FileIncoming &file = it->second;

			packString(buf, file.getTmpName() );
			packString(buf, file.getName() );
			packString(buf, file.getType() );
			packNumber(buf, file.getSize() );
		}
	}

	const uint8_t *unpackFilesIncoming(
		std::unordered_multimap<std::string, Transfer::FileIncoming> &map,
		const uint8_t *src
	) {
		size_t count;
		src = unpackNumber(&count, src);

		for (size_t i = 0; i < count; ++i) {
			std::string key;
			src = unpackString(key, src);

			std::string file_tmp_name;
			src = unpackString(file_tmp_name, src);

			std::string file_name;
			src = unpackString(file_name, src);

			std::string file_type;
			src = unpackString(file_type, src);

			size_t file_size;
			src = unpackNumber(&file_size, src);

			map.emplace(
				std::move(key),
				Transfer::FileIncoming(
					std::move(file_tmp_name),
					std::move(file_name),
					std::move(file_type),
					file_size
				)
			);
		}

		return src;
	}
}
