#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace Transfer
{
	class FileIncoming
	{
	protected:
		std::string file_tmp_name;
		std::string file_name;
		std::string file_type;
		size_t file_size;

	public:
		FileIncoming() = default;

		FileIncoming(
			std::string &&fileTmpName,
			std::string &&fileName,
			std::string &&fileType,
			const size_t fileSize
		) noexcept;

		FileIncoming(const FileIncoming &obj);
		FileIncoming(FileIncoming &&obj) noexcept;

		~FileIncoming() noexcept = default;

		FileIncoming &operator =(const FileIncoming &) = default;

		const std::string &getTmpName() const noexcept;
		const std::string &getName() const noexcept;
		const std::string &getType() const noexcept;
		size_t getSize() const noexcept;

		bool isExists() const noexcept;
	};
}

namespace Utils
{
	void packFilesIncoming(
		std::vector<char> &buf,
		const std::unordered_multimap<std::string, Transfer::FileIncoming> &map
	);

	const uint8_t *unpackFilesIncoming(
		std::unordered_multimap<std::string, Transfer::FileIncoming> &map,
		const uint8_t *src
	);
}
