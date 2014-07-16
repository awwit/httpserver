
#include "DataVariantFormUrlencoded.h"

namespace HttpServer
{
	DataVariantFormUrlencoded::DataVariantFormUrlencoded()
	{
		data_variant_name = "application/x-www-form-urlencoded";
	}

	bool DataVariantFormUrlencoded::parse
	(
		const Socket &sock,
		const std::chrono::milliseconds &timeout,
		const std::string &str,
		const size_t leftBytes,
		const std::unordered_map<std::string, std::string> &params,
		std::unordered_multimap<std::string, std::string> &data,
		std::unordered_multimap<std::string, FileIncoming> &files
	)
	{
		if (str.length() )
		{
			for (size_t var_pos = 0, var_end; std::string::npos != var_pos; var_pos = var_end)
			{
				// Поиск следующего параметра
				var_end = str.find('&', var_pos);

				// Поиск значения параметра
				size_t delimiter = str.find('=', var_pos);

				if (std::string::npos == delimiter || delimiter > var_end)
				{
					return false;
				}

				// Получить имя параметра
				std::string var_name = str.substr(var_pos, delimiter - var_pos);

				++delimiter;

				std::string var_value;

				// Если последний параметр
				if (std::string::npos == var_end)
				{
					var_value = str.substr(delimiter);
				}
				else // Если не последний параметр
				{
					var_value = str.substr(delimiter, var_end - delimiter);
				}

				// Сохранить параметр и значение
				data.emplace(std::move(var_name), std::move(var_value) );

				if (std::string::npos != var_end)
				{
					++var_end;
				}
			}

			return true;
		}

		return false;
	}
};