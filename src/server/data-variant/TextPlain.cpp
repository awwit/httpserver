
#include "TextPlain.h"

namespace DataVariant
{
	TextPlain::TextPlain() noexcept {
		this->data_variant_name = "text/plain";
	}

	bool TextPlain::parse(const std::string &buf, Transfer::request_data *rd, DataReceiver *dr) const
	{
		if (buf.empty() ) {
			return 0 == dr->full_size || dr->full_size != dr->recv_total;
		}

		for (
			size_t var_pos = 0, var_end = 0;
			std::string::npos != var_end;
			var_pos = var_end + 1
		) {
			// Поиск следующего параметра
			var_end = buf.find('&', var_pos);

			if (std::string::npos == var_end) {
				if (dr->full_size != dr->recv_total) {
					dr->left = buf.size() - var_pos;
					return true;
				}
			}

			// Поиск значения параметра
			size_t delimiter = buf.find('=', var_pos);

			if (delimiter >= var_end)
			{
				// Получить имя параметра
				std::string var_name = buf.substr(
					var_pos,
					std::string::npos != var_end
						? var_end - var_pos
						: std::string::npos
				);

				// Сохранить параметр с пустым значением
				rd->incoming_data.emplace(
					std::move(var_name),
					std::string()
				);
			} else {
				// Получить имя параметра
				std::string var_name = buf.substr(var_pos, delimiter - var_pos);

				++delimiter;

				// Получить значение параметра
				std::string var_value = buf.substr(
					delimiter,
					std::string::npos != var_end
						? var_end - delimiter
						: std::string::npos
				);

				// Сохранить параметр и значение
				rd->incoming_data.emplace(
					std::move(var_name),
					std::move(var_value)
				);
			}
		}

		return true;
	}
}
