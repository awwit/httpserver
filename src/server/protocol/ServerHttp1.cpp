
#include "ServerHttp1.h"

#include "extensions/Sendfile.h"
#include "ServerWebSocket.h"

#include "../../utils/Utils.h"

#include <algorithm>

namespace HttpServer
{
	ServerHttp1::ServerHttp1(
		Socket::Adapter &sock,
		const ServerSettings &settings,
		ServerControls &controls
	) noexcept
		: ServerProtocol(sock, settings, controls)
	{

	}

	bool ServerHttp1::sendHeaders(
		const Http::StatusCode status,
		std::vector<std::pair<std::string, std::string> > &headers,
		const std::chrono::milliseconds &timeout,
		const bool endStream
	) const {
		static const std::unordered_map<int, std::string> status_list {
			{ 200, "OK" },
			{ 206, "Partial Content" },
			{ 304, "Not Modified" },
			{ 400, "Bad Request" },
			{ 404, "Not Found" },
			{ 413, "Request Entity Too Large" },
			{ 416, "Requested Range Not Satisfiable" },
			{ 500, "Internal Server Error" },
		};

		std::string str = "HTTP/1.1 " + std::to_string(static_cast<int>(status) );

		auto const it = status_list.find(static_cast<int>(status) );

		if (status_list.cend() != it) {
			const std::string &status = it->second;
			str += ' ' + status;
		}

		str += "\r\n";

		for (auto const &header : headers) {
			str += header.first + ": " + header.second + "\r\n";
		}

		str += "\r\n";

		return this->sock.nonblock_send(str, timeout) > 0; // >= 0
	}

	long ServerHttp1::sendData(
		const void *src,
		size_t size,
		const std::chrono::milliseconds &timeout,
		DataTransfer *dt
	) const {
		const long send_size = this->sock.nonblock_send(
			src,
			size,
			timeout
		);

		if (send_size > 0) {
			dt->send_total += static_cast<size_t>(send_size);
		}

		return send_size;
	}

	void ServerHttp1::close() {
		this->sock.close();
	}

	bool ServerHttp1::packRequestParameters(
		std::vector<char> &buf,
		const struct Request &req,
		const std::string &rootDir
	) const {
		Utils::packNumber(buf, static_cast<size_t>(Transfer::ProtocolVariant::HTTP_1) );
		Utils::packString(buf, rootDir);
		Utils::packString(buf, req.host);
		Utils::packString(buf, req.path);
		Utils::packString(buf, req.method);
		Utils::packContainer(buf, req.incoming_headers);
		Utils::packContainer(buf, req.incoming_data);
		Utils::packFilesIncoming(buf, req.incoming_files);

		return true;
	}

	void ServerHttp1::unpackResponseParameters(
		struct Request &req,
		const void *src
	) const {
		Utils::unpackContainer(
			req.outgoing_headers,
			reinterpret_cast<const uint8_t *>(src)
		);
	}

	static bool getRequest(
		const Socket::Adapter &sock,
		struct Request &req,
		std::vector<char> &buf,
		std::string &str_buf
	) {
		// Получить данные запроса от клиента
		const long recv_size = sock.nonblock_recv(
			buf,
			req.timeout
		);

		if (recv_size < 0 && str_buf.empty() ) {
			return false;
		}

		if (recv_size > 0) { // Если данные были получены
			str_buf.append(
				buf.cbegin(),
				buf.cbegin() + recv_size
			);
		}

		return true;
	}

	static Http::StatusCode getRequestHeaders(
		struct Request &req,
		std::string &str_buf
	) {
		// Если запрос пустой
		if (str_buf.empty() ) {
			return Http::StatusCode::BAD_REQUEST;
		}

		// Поиск конца заголовков (пустая строка)
		size_t headers_end = str_buf.find("\r\n\r\n");

		// Если найден конец заголовков
		if (std::string::npos == headers_end) {
			return Http::StatusCode::BAD_REQUEST;
		}

		headers_end += 2;

		size_t str_cur = 0;
		// Поиск конца первого заголовка
		size_t str_end = str_buf.find("\r\n");

		// Если не найден конец заголовка
		if (std::string::npos == str_end) {
			return Http::StatusCode::BAD_REQUEST;
		}

		// Установка конца строки (для поиска)
		str_buf[str_end] = '\0';

		// Разделить метод запроса и параметры запроса
		size_t delimiter = str_buf.find(' ', str_cur);

		// Получить метод запроса (GET, POST, PUT, DELETE, ...)
		req.method = str_buf.substr(
			str_cur,
			delimiter - str_cur
		);

		Utils::toLower(req.method);

		// Сохранить метод и параметры запроса
	//	rp.incoming_headers[rp.method] = str_buf.substr(delimiter + 1, str_end - delimiter - 1);

		delimiter += 1;
		// Найти окончание URI
		size_t uri_end = str_buf.find(' ', delimiter);

		// Если окончание не найдено
		if (std::string::npos == uri_end) {
			uri_end = str_end;
			// то версия протокола HTTP - 0.9
		//	const std::string version = "0.9";
		} else {
			// Если окончание найдено
			str_buf[uri_end] = '\0';
			const size_t ver_beg = uri_end + 6; // Пропустить "HTTP/"

			if (ver_beg < str_end) {
				// Получить версию протокола HTTP
			//	const std::string version = str_buf.substr(ver_beg, str_end - ver_beg);
			}
		}

		// Сохранить полную ссылку URI
		req.path = str_buf.substr(delimiter, uri_end - delimiter);

		// Переход к обработке следующего заголовка
		str_cur = str_end + 2;
		// Поиск конца заголовка
		str_end = str_buf.find("\r\n", str_cur);
		// Установка конца заголовка
		str_buf[str_end] = '\0';

		// Цикл извлечения заголовков запроса
		for (
			 ;
			 str_cur != headers_end;
			 str_end = str_buf.find("\r\n", str_cur), str_buf[str_end] = '\0'
		) {
			// Поиск разделителя названия заголовка и его значения
			delimiter = str_buf.find(':', str_cur);

			// Если разделитель найден в текущей строке
			if (delimiter < str_end)
			{
				std::string header_name = str_buf.substr(
					str_cur,
					delimiter - str_cur
				);

				Utils::toLower(header_name);

				std::string header_value = str_buf.substr(
					delimiter + 1,
					str_end - delimiter - 1
				);

				// Удалить лишние пробелы в начале и в конце строки
				Utils::trim(header_value);

				// Сохранить заголовок и его значение
				req.incoming_headers.emplace(
					std::move(header_name),
					std::move(header_value)
				);
			}

			// Перейти к следующей строке
			str_cur = str_end + 2;
		}

		str_buf.erase(0, headers_end + 2);

		return Http::StatusCode::EMPTY;
	}

	const ServerApplicationSettings *
	ServerHttp1::getApplicationSettings(
		struct Request &req,
		const bool isSecureConnection
	) const {
		// Получить доменное имя (или адрес) назначения запроса
		auto const it_host = req.incoming_headers.find("host");

		// Если имя задано - продолжить обработку запроса
		if (req.incoming_headers.cend() != it_host)
		{
			const std::string &host_header = it_host->second;

			// Поиск разделителя, за которым помещается номер порта, если указан
			const size_t delimiter = host_header.find(':');

			// Получить имя (или адрес)
			req.host = host_header.substr(0, delimiter);

			const int default_port = isSecureConnection ? 443 : 80;

			// Получить номер порта
			const int port = (std::string::npos != delimiter)
				? std::atoi(host_header.substr(delimiter + 1).c_str())
				: default_port;

			// Поиск настроек приложения по имени
			const ServerApplicationSettings *app_sets = this->settings.apps_tree.find(req.host);

			// Если приложение найдено
			if (app_sets && (
					app_sets->ports.cend() != app_sets->ports.find(port) ||
					app_sets->tls_ports.cend() != app_sets->tls_ports.find(port)
				)
			) {
				return app_sets;
			}
		}

		return nullptr;
	}

	Http::StatusCode ServerHttp1::getRequestData(
		struct Request &req,
		std::string &str_buf,
		const ServerApplicationSettings &appSets
	) const {
		// Определить вариант данных запроса (заодно проверить, есть ли данные)
		auto const it = req.incoming_headers.find("content-type");

		if (req.incoming_headers.cend() == it) {
			return Http::StatusCode::EMPTY;
		}

		// Получить значение заголовка
		const std::string &header_value = it->second;

		std::string data_variant_name; // Название варианта данных запроса

		std::unordered_map<std::string, std::string> content_params;

		// Определить, содержит ли тип данных запроса дополнительные параметры
		size_t delimiter = header_value.find(';');

		// Если есть дополнительные параметры - извлекаем их
		if (std::string::npos != delimiter)
		{
			data_variant_name = header_value.substr(0, delimiter);
			Utils::trim(data_variant_name);

			for (
				 size_t str_param_cur = delimiter + 1, str_param_end = 0;
				 std::string::npos != str_param_end;
				 str_param_cur = str_param_end + 1
			) {
				str_param_end = header_value.find(';', str_param_cur);
				delimiter = header_value.find('=', str_param_cur);

				if (delimiter >= str_param_end)
				{
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
				}
				else
				{
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

		// Поиск варианта данных по имени типа
		auto const variant = this->settings.variants.find(data_variant_name);

		// Если сервер не поддерживает формат полученных данных
		if (this->settings.variants.cend() == variant) {
			return Http::StatusCode::BAD_REQUEST;
		}

		const DataVariant::Abstract *data_variant = variant->second;

		// Получить длину запроса в байтах
		size_t data_length = 0;

		auto const it_len = req.incoming_headers.find("content-length");

		if (req.incoming_headers.cend() != it_len) {
			data_length = std::strtoull(
				it_len->second.c_str(),
				nullptr,
				10
			);
		}

		// Если размер запроса превышает лимит (если лимит был установлен)
		if (data_length > appSets.request_max_size && 0 != appSets.request_max_size) {
			return Http::StatusCode::REQUEST_ENTITY_TOO_LARGE;
		}

		Transfer::request_data *rd = static_cast<Transfer::request_data *>(&req);

		DataVariant::DataReceiver dr {
			data_variant,
			data_variant->createStateStruct(rd, content_params),
			data_length,
			0, 0, nullptr,
		};

		std::string data_buf;

		if (str_buf.length() <= data_length) {
			dr.recv_total = str_buf.length();
			data_buf.swap(str_buf);
		} else {
			data_buf.assign(
				str_buf, 0, data_length
			);

			str_buf.erase(0, data_length);

			dr.recv_total = data_buf.size();
		}

		bool result = data_variant->parse(data_buf, rd, &dr);

		while (result && dr.full_size > dr.recv_total)
		{
			std::vector<char> buf(
				dr.full_size - dr.recv_total >= 512 * 1024
					? 512 * 1024
					: dr.full_size - dr.recv_total
			);

			long recv_size = this->sock.nonblock_recv(
				buf.data(),
				buf.size(),
				req.timeout
			);

			if (recv_size <= 0) {
				result = false;
				break;
			}

			dr.recv_total += static_cast<size_t>(recv_size);

			data_buf.erase(
				0, data_buf.length() - dr.left
			);

			data_buf.append(
				buf.data(),
				0, static_cast<size_t>(recv_size)
			);

			dr.left = 0;

			result = data_variant->parse(data_buf, rd, &dr);
		}

		data_variant->destroyStateStruct(dr.ss);

		if (false == result) {
			for (auto const &it : req.incoming_files) {
				std::remove(it.second.getTmpName().c_str() );
			}

			return Http::StatusCode::BAD_REQUEST;
		}

		if (dr.left) {
			str_buf.assign(
				data_buf,
				data_buf.length() - dr.left,
				data_buf.length()
			);
		}

		return Http::StatusCode::EMPTY;
	}

	static void sendStatus(
		const Socket::Adapter &sock,
		const struct Request &req,
		const Http::StatusCode statusCode
	) {
		static const std::unordered_map<int, std::string> status_list {
			{ 400, "Bad Request" },
			{ 404, "Not Found" },
			{ 413, "Request Entity Too Large" },
		};

		auto const it = status_list.find(static_cast<int>(statusCode) );

		if (status_list.cend() != it) {
			const std::string &status = it->second;

			std::string headers("HTTP/1.1 " + std::to_string(static_cast<int>(statusCode) ) + ' ' + status + "\r\n\r\n");

			sock.nonblock_send(headers, req.timeout);
		}
	}

	static void getConnectionParams(
		struct Request &req,
		const bool isSecureConnection
	) {
		auto const it_in_connection = req.incoming_headers.find("connection");
		auto const it_out_connection = req.outgoing_headers.find("connection");

		if (
			req.incoming_headers.cend() != it_in_connection &&
			req.outgoing_headers.cend() != it_out_connection
		) {
			const std::string connection_in = Utils::getLowerString(it_in_connection->second);
			const std::string connection_out = Utils::getLowerString(it_out_connection->second);

			auto const incoming_params = Utils::explode(connection_in, ',');

			auto const it = std::find(
				incoming_params.cbegin(),
				incoming_params.cend(),
				connection_out
			);

			if (incoming_params.cend() != it)
			{
				const std::string &inc = *it;

				if ("keep-alive" == inc)
				{
					--req.keep_alive_count;

					if (0 < req.keep_alive_count) {
						req.connection_params |= ConnectionParams::CONNECTION_REUSE;
					}
				}
				else if ("upgrade" == inc)
				{
					auto const it_out_upgrade = req.outgoing_headers.find("upgrade");

					if (req.outgoing_headers.cend() != it_out_upgrade)
					{
						const std::string upgrade = Utils::getLowerString(it_out_upgrade->second);

						if ("h2" == upgrade) {
							if (isSecureConnection) {
								req.protocol_variant = Transfer::ProtocolVariant::HTTP_2;
								req.connection_params |= ConnectionParams::CONNECTION_REUSE;
							}
						}
						else if ("h2c" == upgrade) {
							if (false == isSecureConnection) {
								req.protocol_variant = Transfer::ProtocolVariant::HTTP_2;
								req.connection_params |= ConnectionParams::CONNECTION_REUSE;
							}
						}
						else if ("websocket" == upgrade) {
							req.connection_params |= ConnectionParams::CONNECTION_LEAVE_OPEN;
						}
					}
				}
			}
		}
	}

	void ServerHttp1::useHttp1Protocol(
		struct Request &req,
		std::vector<char> &buf,
		std::string &str_buf
	) const {
		if (getRequest(this->sock, req, buf, str_buf) == false) {
			return;
		}

		Http::StatusCode error_code = getRequestHeaders(req, str_buf);

		if (error_code != Http::StatusCode::EMPTY) {
			sendStatus(this->sock, req, error_code);
			return;
		}

		const ServerApplicationSettings *app_sets = this->getApplicationSettings(
			req,
			this->sock.get_tls_session() != nullptr
		);

		// Если приложение не найдено
		if (nullptr == app_sets) {
			sendStatus(this->sock, req, Http::StatusCode::NOT_FOUND);
			return;
		}

		error_code = this->getRequestData(req, str_buf, *app_sets);

		if (error_code != Http::StatusCode::EMPTY) {
			sendStatus(this->sock, req, error_code);
			return;
		}

		this->runApplication(req, *app_sets);

		for (auto const &it : req.incoming_files) {
			std::remove(it.second.getTmpName().c_str() );
		}

		if (EXIT_SUCCESS == req.app_exit_code) {
			getConnectionParams(
				req,
				this->sock.get_tls_session() != nullptr
			);

			Sendfile::xSendfile(
				std::ref(*this),
				req,
				this->settings.mimes_types
			);
		}
	}

	static bool isConnectionLeaveOpen(const struct Request &req) {
		return (req.connection_params & ConnectionParams::CONNECTION_LEAVE_OPEN) == ConnectionParams::CONNECTION_LEAVE_OPEN;
	}

	ServerProtocol *ServerHttp1::process()
	{
		struct Request req;
		req.timeout = std::chrono::milliseconds(5000);
		req.protocol_variant = Transfer::ProtocolVariant::HTTP_1;
		req.keep_alive_count = 100;

		std::vector<char> buf(4096);
		std::string str_buf;

		do {
			// Подготовить параметры для получения данных
			req.connection_params = ConnectionParams::CONNECTION_CLOSE;
			req.app_exit_code = EXIT_FAILURE;

			this->useHttp1Protocol(req, buf, str_buf);

			req.clear();
		}
		while (Sendfile::isConnectionReuse(req) );

		if (isConnectionLeaveOpen(req) ) {
			return new ServerWebSocket(*this);
		}

		return this;
	}
}
