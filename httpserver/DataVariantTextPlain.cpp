
#include "DataVariantTextPlain.h"

namespace HttpServer
{
	DataVariantTextPlain::DataVariantTextPlain()
	{
		data_variant_name = "text/plain";
	}

	bool DataVariantTextPlain::parse
	(
		const Socket &sock,
		const std::chrono::milliseconds &timeout,
		std::string &str,
		const size_t leftBytes,
		const std::unordered_map<std::string, std::string> &params,
		std::unordered_multimap<std::string, std::string> &data,
		std::unordered_multimap<std::string, FileIncoming> &files
	)
	{
		if (str.length() )
		{
			for (size_t var_pos = 0, var_end = 0; std::string::npos != var_end; var_pos = var_end + 1)
			{
				// Поиск следующего параметра
				var_end = str.find('&', var_pos);

				// Поиск значения параметра
				size_t delimiter = str.find('=', var_pos);

				if (delimiter >= var_end)
				{
					// Получить имя параметра
					std::string var_name = str.substr(var_pos, std::string::npos != var_end ? var_end - var_pos : std::string::npos);

					// Сохранить параметр с пустым значением
					data.emplace(std::move(var_name), "");
				}
				else
				{
					// Получить имя параметра
					std::string var_name = str.substr(var_pos, delimiter - var_pos);

					++delimiter;

					// Получить значение параметра
					std::string var_value = str.substr(delimiter, std::string::npos != var_end ? var_end - delimiter : std::string::npos);

					// Сохранить параметр и значение
					data.emplace(std::move(var_name), std::move(var_value) );
				}
			}

			str.clear();

			return true;
		}

		return false;
	}
};