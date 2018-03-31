
#include "ServerProtocol.h"
#include "../../utils/Utils.h"

namespace HttpServer
{
	ServerProtocol::ServerProtocol(
		Socket::Adapter &sock,
		const ServerSettings &settings,
		ServerControls &controls
	) noexcept
		: sock(sock), settings(settings), controls(controls)
	{

	}

	ServerProtocol::ServerProtocol(const ServerProtocol &prot) noexcept
		: sock(prot.sock), settings(prot.settings), controls(prot.controls)
	{

	}

	DataVariant::DataReceiver *
	ServerProtocol::createDataReceiver(
		const Transfer::request_data *rd,
		const std::unordered_map<std::string,
		DataVariant::Abstract *> &variants
	) {
		auto const it = rd->incoming_headers.find("content-type");

		if (rd->incoming_headers.cend() == it) {
			return nullptr;
		}

		// Получить значение заголовка
		const std::string &header_value = it->second;

		std::string data_variant_name; // Название варианта данных запроса

		std::unordered_map<std::string, std::string> content_params;

		// Определить, содержит ли тип данных запроса дополнительные параметры
		size_t delimiter = header_value.find(';');

		// Если есть дополнительные параметры - извлекаем их
		if (std::string::npos != delimiter) {
			data_variant_name = header_value.substr(0, delimiter);
			Utils::trim(data_variant_name);

			for (
				 size_t str_param_cur = delimiter + 1, str_param_end = 0;
				 std::string::npos != str_param_end;
				 str_param_cur = str_param_end + 1
			) {
				str_param_end = header_value.find(';', str_param_cur);
				delimiter = header_value.find('=', str_param_cur);

				if (delimiter >= str_param_end) {
					std::string param_name = header_value.substr(
						str_param_cur,
						std::string::npos != str_param_end
							? str_param_end - str_param_cur
							: std::string::npos
					);

					Utils::trim(param_name);

					content_params.emplace(
						std::move(param_name),
						std::string()
					);
				} else {
					std::string param_name = header_value.substr(
						str_param_cur,
						delimiter - str_param_cur
					);

					Utils::trim(param_name);

					++delimiter;

					std::string param_value = header_value.substr(
						delimiter,
						std::string::npos != str_param_end
							? str_param_end - delimiter
							: std::string::npos
					);

					Utils::trim(param_value);

					content_params.emplace(
						std::move(param_name),
						std::move(param_value)
					);
				}
			}
		} else {
			data_variant_name = header_value;
		}

		auto const variant = variants.find(data_variant_name);

		if (variants.cend() == variant) {
			return nullptr;
		}

		const DataVariant::Abstract *data_variant = variant->second;

		size_t data_length = 0;

		auto const it_len = rd->incoming_headers.find("content-length");

		if (rd->incoming_headers.cend() != it_len) {
			data_length = std::strtoull(
				it_len->second.c_str(),
				nullptr,
				10
			);
		}

		return new DataVariant::DataReceiver {
			data_variant,
			data_variant->createStateStruct(rd, content_params),
			data_length,
			0, 0, nullptr,
		};
	}

	void ServerProtocol::destroyDataReceiver(void *src)
	{
		DataVariant::DataReceiver *dr = reinterpret_cast<DataVariant::DataReceiver *>(src);

		if (dr) {
			dr->data_variant->destroyStateStruct(dr->ss);

			if (dr->reserved) {
				delete reinterpret_cast<std::string *>(dr->reserved);
				dr->reserved = nullptr;
			}
		}

		delete dr;
	}

	void ServerProtocol::runApplication(
		struct Request &req,
		const ServerApplicationSettings &appSets
	) const {
		std::vector<char> buf;
		buf.reserve(4096);

		if (this->packRequestParameters(buf, req, appSets.root_dir) == false) {
			return;
		}

		Transfer::app_request request {
			this->sock.get_handle(),
			this->sock.get_tls_session(),
			buf.data()
		};

		Transfer::app_response response {
			nullptr, 0
		};

		try {
			// Launch application
			req.app_exit_code = appSets.application_call(&request, &response);
		}
		catch (std::exception &exc) {
			// TODO: exception output
		}

		if (response.response_data && response.data_size)
		{
			if (EXIT_SUCCESS == req.app_exit_code) {
				this->unpackResponseParameters(req, response.response_data);
			}

			// Clear outgoing data of application
			try {
				appSets.application_clear(response.response_data, response.data_size);
			}
			catch (std::exception &exc) {
				// TODO: exception output
			}
		}
	}
}
