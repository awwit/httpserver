
#include "DataVariantTextPlain.h"

namespace HttpServer
{
	DataVariantTextPlain::DataVariantTextPlain()
	{
		data_variant_name = "text/plain";
	}

	bool DataVariantTextPlain::parse
	(
		const SocketAdapter &sock,
		std::string &str,
		const size_t leftBytes,
		std::unordered_map<std::string, std::string> &contentParams,
		struct request_parameters &rp
	)
	{
		if (str.empty() )
		{
			return true;
		}

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
				rp.incoming_data.emplace(std::move(var_name), "");
			}
			else
			{
				// Получить имя параметра
				std::string var_name = str.substr(var_pos, delimiter - var_pos);

				++delimiter;

				// Получить значение параметра
				std::string var_value = str.substr(delimiter, std::string::npos != var_end ? var_end - delimiter : std::string::npos);

				// Сохранить параметр и значение
				rp.incoming_data.emplace(std::move(var_name), std::move(var_value) );
			}
		}

		str.clear();

		return true;
	}
};