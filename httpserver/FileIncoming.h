#pragma once

#include <string>

namespace HttpServer
{
	class FileIncoming
	{
	protected:
		std::string file_name;
		std::string file_type;
		size_t file_size;

	private:
		FileIncoming() = delete;

	public:
		FileIncoming(const std::string &, const std::string &, const size_t);
		~FileIncoming() = default;

		inline std::string getName() const
		{
			return file_name;
		}

		inline std::string getType() const
		{
			return file_type;
		}

		inline size_t getSize() const
		{
			return file_size;
		}

		bool isExists() const;
	};
};
