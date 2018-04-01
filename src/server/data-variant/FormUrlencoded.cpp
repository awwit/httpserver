
#include "FormUrlencoded.h"

#include "../../utils/Utils.h"

namespace DataVariant
{
	FormUrlencoded::FormUrlencoded() noexcept {
		this->data_variant_name = "application/x-www-form-urlencoded";
	}

	bool FormUrlencoded::parse(const std::string &buf, Transfer::request_data *rd, DataReceiver *dr) const
	{
		if (buf.empty() ) {
			return 0 == dr->full_size || dr->full_size != dr->recv_total;
		}

		for (
			size_t var_pos = 0, var_end = 0;
			std::string::npos != var_end;
			var_pos = var_end + 1
		) {
			// Search next parameter
			var_end = buf.find('&', var_pos);

			if (std::string::npos == var_end) {
				if (dr->full_size != dr->recv_total) {
					dr->left = buf.size() - var_pos;
					return true;
				}
			}

			// Search parameter value
			size_t delimiter = buf.find('=', var_pos);

			if (delimiter >= var_end) {
				// Get parameter name
				std::string var_name = Utils::urlDecode(
					buf.substr(
						var_pos,
						std::string::npos != var_end
							? var_end - var_pos
							: std::string::npos
					)
				);

				// Store parameter with empty value
				rd->incoming_data.emplace(
					std::move(var_name),
					std::string()
				);
			} else {
				// Get parameter name
				std::string var_name = Utils::urlDecode(
					buf.substr(
						var_pos,
						delimiter - var_pos
					)
				);

				++delimiter;

				// Get parameter value
				std::string var_value = Utils::urlDecode(
					buf.substr(
						delimiter,
						std::string::npos != var_end
							? var_end - delimiter
							: std::string::npos
					)
				);

				// Store parameter and value
				rd->incoming_data.emplace(
					std::move(var_name),
					std::move(var_value)
				);
			}
		}

		return true;
	}
}
