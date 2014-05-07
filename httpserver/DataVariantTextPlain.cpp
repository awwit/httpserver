
#include "DataVariantTextPlain.h"

namespace HttpServer
{
	DataVariantTextPlain::DataVariantTextPlain()
	{
		data_variant_name = "text/plain";
	}

	bool DataVariantTextPlain::parse
	(
		const Socket *sock,
		const std::string str,
		const size_t leftBytes,
		const std::unordered_map<std::string, std::string> &params,
		std::unordered_multimap<std::string, std::string> &data,
		std::unordered_multimap<std::string, FileIncoming> &files
	)
	{
		if (str.length() )
		{
			size_t var_pos = 0;
			size_t var_end = std::string::npos;

			std::string var_name;
			std::string var_value;

			for (; std::string::npos != var_pos; var_pos = var_end)
			{
				// Поиск следующего параметра
				var_end = str.find('&', var_pos);

				// Поиск значения параметра
				size_t delimiter = str.find('=', var_pos);

				if (std::string::npos != delimiter)
				{
					// Получить имя параметра
					var_name = str.substr(var_pos, delimiter - var_pos);

					++delimiter;

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
					data.emplace(var_name, var_value);

					if (std::string::npos != var_end)
					{
						++var_end;
					}
				}
			}

			return true;
		}

		return false;
	}
};
