
#include "Server.h"
#include "Utils.h"
#include "System.h"

#include "DataVariantFormUrlencoded.h"
#include "DataVariantMultipartFormData.h"
#include "DataVariantTextPlain.h"
#include "FileIncoming.h"
#include "ServerRequest.h"
#include "ServerResponse.h"
#include "ConfigParser.h"

#include <iostream>
#include <iomanip>
#include <map>
#include <thread>
#include <fstream>
#include <cstring>

namespace HttpServer
{
	std::string Server::getMimeTypeByFileName(const std::string &fileName) const
	{
		const size_t ext_pos = fileName.rfind('.');
		std::string file_ext = std::string::npos != ext_pos ? fileName.substr(ext_pos + 1) : "";

		std::locale loc;
		Utils::tolower(file_ext, loc);

		auto it_mime = this->mimes_types.find(file_ext);

		return this->mimes_types.cend() != it_mime ? it_mime->second : "application/octet-stream";
	}

	std::vector<std::tuple<size_t, size_t> > Server::getRanges(
		const std::string &rangeHeader,
		const size_t posSymEqual,
		const size_t fileSize,
		std::string &resultRangeHeader,
		size_t &contentLength
	) const
	{
		std::vector<std::tuple<size_t, size_t> > ranges;

		contentLength = 0;

		size_t delimiter = posSymEqual; // rangeHeader.find('=');

		const std::string range_unit_name(rangeHeader.cbegin(), rangeHeader.cbegin() + delimiter);

		static const std::unordered_map<std::string, size_t> ranges_units {
			{"bytes", 1}
		};

		auto it_unit = ranges_units.find(range_unit_name);

		if (ranges_units.cend() == it_unit)
		{
			return ranges;
		}

		const size_t &range_unit = it_unit->second;

		for (size_t str_pos; std::string::npos != delimiter; )
		{
			str_pos = delimiter + 1;

			delimiter = rangeHeader.find(',', str_pos);

			size_t range_pos = rangeHeader.find('-', str_pos);

			if (range_pos < delimiter)
			{
				const std::string range_begin_str(rangeHeader.cbegin() + str_pos, rangeHeader.cbegin() + range_pos);
				const std::string range_end_str(rangeHeader.cbegin() + range_pos + 1, std::string::npos == delimiter ? rangeHeader.cend() : rangeHeader.cbegin() + delimiter);

				if (false == range_begin_str.empty() )
				{
					const size_t range_begin = std::strtoull(range_begin_str.c_str(), nullptr, 10) * range_unit;

					if (range_begin < fileSize)
					{
						if (false == range_end_str.empty() )
						{
							size_t range_end = std::strtoull(range_end_str.c_str(), nullptr, 10) * range_unit;

							if (range_end >= range_begin)
							{
								if (range_end > fileSize)
								{
									range_end = fileSize;
								}

								const size_t length = range_end - range_begin + 1;

								contentLength += length;

								resultRangeHeader += std::to_string(range_begin) + '-' + std::to_string(range_end) + ',';

								ranges.emplace_back(std::tuple<size_t, size_t> {range_begin, length});
							}
						}
						else // if range_end_str empty
						{
							const size_t length = fileSize - range_begin;

							contentLength += length;

							resultRangeHeader += std::to_string(range_begin) + '-' + std::to_string(fileSize - 1) + ',';

							ranges.emplace_back(std::tuple<size_t, size_t> {range_begin, length});
						}
					}
				}
				else if (false == range_end_str.empty() ) // if range_begin_str empty
				{
					size_t range_end = std::strtoull(range_end_str.c_str(), nullptr, 10) * range_unit;

					const size_t length = range_end < fileSize ? fileSize - range_end : fileSize;

					const size_t range_begin = fileSize - length;

					range_end = fileSize - range_begin - 1;

					contentLength += length;

					resultRangeHeader += std::to_string(range_begin) + '-' + std::to_string(range_end) + ',';

					ranges.emplace_back(std::tuple<size_t, size_t> {range_begin, length});
				}
			}
		}

		if (false == ranges.empty() )
		{
			resultRangeHeader.back() = '/';

			resultRangeHeader = "bytes " + resultRangeHeader + std::to_string(fileSize);
		}

		return ranges;
	}

	int Server::transferFilePart
	(
		const Socket &clientSocket,
		const std::chrono::milliseconds &timeout,
		const std::string &fileName,
		const time_t fileTime,
		const size_t fileSize,
		const std::string &rangeHeader,
		const std::string &connectionHeader,
		const std::string &dateHeader,
		const bool headersOnly
	) const
	{
		const size_t pos_sym_equal = rangeHeader.find('=');

		if (std::string::npos == pos_sym_equal)
		{
			// HTTP 400
			std::string headers("HTTP/1.1 400 Bad Request\r\n");
			headers += connectionHeader + dateHeader + "\r\n";

			clientSocket.nonblock_send(headers, timeout);

			return 0;
		}

		std::string content_range_header;

		size_t content_length;

		const std::vector<std::tuple<size_t, size_t> > ranges = this->getRanges(rangeHeader, pos_sym_equal, fileSize, content_range_header, content_length);

		if (0 == content_length)
		{
			// HTTP 416
			std::string headers("HTTP/1.1 416 Requested Range Not Satisfiable\r\n");
			headers += connectionHeader + dateHeader + "\r\n";

			clientSocket.nonblock_send(headers, timeout);

			return 0;
		}

		// Ranges transfer
		std::ifstream file(fileName, std::ifstream::binary);

		if ( ! file)
		{
			file.close();

			// HTTP 500
			std::string headers("HTTP/1.1 500 Internal Server Error\r\n");
			headers += connectionHeader + dateHeader + "\r\n";

			clientSocket.nonblock_send(headers, timeout);

			return 0;
		}

		const std::string file_mime_type = this->getMimeTypeByFileName(fileName);

		std::string headers("HTTP/1.1 206 Partial Content\r\n");
		headers += "Content-Type: " + file_mime_type + "\r\n"
			+ "Content-Length: " + std::to_string(content_length) + "\r\n"
			+ "Accept-Ranges: bytes\r\n"
			+ "Content-Range: " + content_range_header + "\r\n"
			+ "Last-Modified: " + Utils::getDatetimeAsString(fileTime, true) + "\r\n"
			+ connectionHeader + dateHeader + "\r\n";

		// Отправить заголовки
        if (std::numeric_limits<size_t>::max() == clientSocket.nonblock_send(headers, timeout) )
		{
			file.close();

			return 0;
		}

		if (false == headersOnly)
		{
			for (auto &range : ranges)
			{
				const size_t length = std::get<1>(range);

                std::vector<std::string::value_type> buf(length < 512 * 1024 ? length : 512 * 1024);

				const size_t position = std::get<0>(range);

				file.seekg(position, file.beg);

				size_t send_size_left = length;

				size_t send_size;

				do
				{
					if (send_size_left < 512 * 1024)
					{
						buf.resize(send_size_left);
					}

					file.read(reinterpret_cast<char *>(buf.data() ), buf.size() );
					send_size = clientSocket.nonblock_send(buf, file.gcount(), timeout);

					send_size_left -= send_size;
				}
				while (false == file.eof() && false == file.fail() && std::numeric_limits<size_t>::max() != send_size && send_size_left);
			}
		}

		file.close();

		return 1;
	}

	/**
	 * Передача файла (или его части)
	 */
	int Server::transferFile
	(
		const Socket &clientSocket,
		const std::string &fileName,
		const std::string &connectionHeader,
		const bool headersOnly,
		struct request_parameters &rp
	) const
	{
		// Get current time in GMT
		const std::string date_header = "Date: " + Utils::getDatetimeAsString() + "\r\n";

		time_t file_time;
		size_t file_size;

		// Получить размер файла и дату последнего изменения
		if (false == System::getFileSizeAndTimeGmt(fileName, &file_size, &file_time) )
		{
			// HTTP 404
			std::string headers("HTTP/1.1 404 Not Found\r\n");
			headers += connectionHeader + date_header + "\r\n";

			clientSocket.nonblock_send(headers, rp.timeout);

			return 0;
		}

		// Check for If-Modified header
		auto it_modified = rp.incoming_headers.find("If-Modified-Since");

		// Если найден заголовок проверки изменения файла (проверить, изменялся ли файл)
		if (rp.incoming_headers.cend() != it_modified)
		{
			const time_t time_in_request = Utils::stringTimeToTimestamp(it_modified->second);

			if (file_time == time_in_request)
			{
				// HTTP 304
				std::string headers("HTTP/1.1 304 Not Modified\r\n");
				headers += connectionHeader + date_header + "\r\n";

				clientSocket.nonblock_send(headers, rp.timeout);

				return 0;
			}
		}

		auto it_range = rp.incoming_headers.find("Range");

		// Range transfer
		if (rp.incoming_headers.cend() != it_range)
		{
			return this->transferFilePart(clientSocket, rp.timeout, fileName, file_time, file_size, it_range->second, connectionHeader, date_header, headersOnly);
		}

		// File transfer
		std::ifstream file(fileName, std::ifstream::binary);

		if ( ! file)
		{
			file.close();

			// HTTP 500
			std::string headers("HTTP/1.1 500 Internal Server Error\r\n");
			headers += connectionHeader + date_header + "\r\n";

			clientSocket.nonblock_send(headers, rp.timeout);

			return 0;
		}

		const std::string file_mime_type = this->getMimeTypeByFileName(fileName);

		std::string headers("HTTP/1.1 200 OK\r\n");
		headers += "Content-Type: " + file_mime_type + "\r\n"
			+ "Content-Length: " + std::to_string(file_size) + "\r\n"
			+ "Accept-Ranges: bytes\r\n"
			+ "Last-Modified: " + Utils::getDatetimeAsString(file_time, true) + "\r\n"
			+ connectionHeader + date_header + "\r\n";

		// Отправить заголовки
		if (std::numeric_limits<size_t>::max() == clientSocket.nonblock_send(headers, rp.timeout) )
		{
			file.close();

			return 0;
		}

		// Отправить файл
		if (false == headersOnly && file_size)
		{
			std::vector<std::string::value_type> buf(file_size < 512 * 1024 ? file_size : 512 * 1024);

			size_t send_size;

			do
			{
				file.read(reinterpret_cast<char *>(buf.data() ), buf.size() );
				send_size = clientSocket.nonblock_send(buf, file.gcount(), rp.timeout);
			}
			while (false == file.eof() && false == file.fail() && std::numeric_limits<size_t>::max() != send_size);
		}

		file.close();

		return 1;
	}

	/**
	 * Парсинг переданных параметров (URI)
	 */
	bool Server::parseIncomingVars(std::unordered_multimap<std::string, std::string> &params, const std::string &uriParams)
	{
		if (uriParams.length() )
		{
			for (size_t var_pos = 0, var_end = 0; std::string::npos != var_end; var_pos = var_end + 1)
			{
				// Поиск следующего параметра
				var_end = uriParams.find('&', var_pos);

				// Поиск значения параметра
				size_t delimiter = uriParams.find('=', var_pos);

				if (delimiter >= var_end)
				{
					// Получить имя параметра
					std::string var_name = Utils::urlDecode(uriParams.substr(var_pos, std::string::npos != var_end ? var_end - var_pos : std::string::npos) );

					// Сохранить параметр с пустым значением
					params.emplace(std::move(var_name), "");
				}
				else
				{
					// Получить имя параметра
					std::string var_name = Utils::urlDecode(uriParams.substr(var_pos, delimiter - var_pos) );

					++delimiter;

					// Получить значение параметра
					std::string var_value = Utils::urlDecode(uriParams.substr(delimiter, std::string::npos != var_end ? var_end - delimiter : std::string::npos) );

					// Сохранить параметр и значение
					params.emplace(std::move(var_name), std::move(var_value) );
				}
			}

			return true;
		}

		return false;
	}

	void Server::sendStatus(const Socket &clientSocket, const std::chrono::milliseconds &timeout, const size_t statusCode)
	{
        static const std::unordered_map<size_t, std::string> statuses {
			{400, "Bad Request"},
			{404, "Not Found"},
			{413, "Request Entity Too Large"}
		};

		auto it = statuses.find(statusCode);

		if (statuses.cend() != it)
		{
			const std::string &status = it->second;

			std::string headers("HTTP/1.1 " + std::to_string(statusCode) + ' ' + status + "\r\n\r\n");

			clientSocket.nonblock_send(headers, timeout);
		}
	}

	/**
	 * Метод для обработки запроса
	 */
	int Server::threadRequestProc(Socket clientSocket) const
	{
		struct request_parameters rp;

		rp.keep_alive_count = 100;

		const size_t buf_len = 4096;
		std::vector<std::string::value_type> buf(buf_len);

		std::string str_buf;

		do
		{
			rp.app_exit_code = EXIT_FAILURE;

			// Подготовить параметры для получения данных
			rp.timeout = std::chrono::milliseconds(5000);

			if (false == getRequest(clientSocket, buf, str_buf, rp) )
			{
				break;
			}

			if (int error_code = getRequestHeaders(str_buf, rp) )
			{
				this->sendStatus(clientSocket, rp.timeout, error_code);

				break;
			}

			const ServerApplicationSettings *app_sets = getApplicationSettings(rp);

			// Если приложение не найдено
			if (nullptr == app_sets)
			{
				// HTTP 404 Not Found
				this->sendStatus(clientSocket, rp.timeout, 404);

				break;
			}

			if (int error_code = getRequestData(clientSocket, str_buf, *app_sets, rp) )
			{
				this->sendStatus(clientSocket, rp.timeout, error_code);

				break;
			}

			runApplication(clientSocket, *app_sets, rp);

			for (auto &it : rp.incoming_files)
			{
				remove(it.second.getName().c_str() );
			}

			if (EXIT_SUCCESS == rp.app_exit_code)
			{
				this->getConnectionParams(rp);

				this->xSendfile(clientSocket, rp);
			}
			else
			{
				rp.connection_params = CONNECTION_CLOSED;
			}

			rp.clear();
		}
		while (isConnectionKeepAlive(rp) );

		if (false == isConnectionUpgrade(rp) )
		{
			// Wait for send all data to client
			clientSocket.nonblock_send_sync();

			clientSocket.shutdown();
			clientSocket.close();
		}

		return rp.app_exit_code;
	}

	bool Server::getRequest(Socket clientSocket, std::vector<std::string::value_type> &buf, std::string &str_buf, struct request_parameters &rp)
	{
		// Получить данные запроса от клиента
		const size_t recv_size = clientSocket.nonblock_recv(buf, rp.timeout);

		if (std::numeric_limits<size_t>::max() == recv_size && str_buf.empty() )
		{
		#ifdef DEBUG
			#ifdef WIN32
				std::cout << "Error: " << WSAGetLastError() << std::endl;
			#elif POSIX
				std::cout << "Error: " << errno << std::endl;
			#endif
		#endif
			return false;
		}

		if (recv_size) // Если данные были получены
		{
			str_buf.append(buf.cbegin(), buf.cbegin() + recv_size);
		}

		return true;
	}

	int Server::getRequestHeaders(std::string &str_buf, struct request_parameters &rp) const
	{
		// Если запрос пустой
		if (str_buf.empty() )
		{
			// HTTP 400 Bad Request
			return 400;
		}

		// Поиск конца заголовков (пустая строка)
		size_t headers_end = str_buf.find("\r\n\r\n");

		// Если найден конец заголовков
		if (std::string::npos == headers_end)
		{
			// HTTP 400 Bad Request
			return 400;
		}

		headers_end += 2;

		size_t str_cur = 0;
		// Поиск конца первого заголовка
		size_t str_end = str_buf.find("\r\n");

		// Если не найден конец заголовка
		if (std::string::npos == str_end)
		{
			// HTTP 400 Bad Request
			return 400;
		}

		// Установка конца строки (для поиска)
		str_buf[str_end] = '\0';

		// Разделить метод запроса и параметры запроса
		size_t delimiter = str_buf.find(' ', str_cur);

		// Получить метод запроса (GET, POST, PUT, DELETE, ...)
		rp.method = str_buf.substr(str_cur, delimiter - str_cur);
		// Сохранить метод и параметры запроса
		rp.incoming_headers[rp.method] = str_buf.substr(delimiter + 1, str_end - delimiter - 1);

		delimiter += 1;
		// Найти окончание URI
		size_t uri_end = str_buf.find(' ', delimiter);

		// Если окончание не найдено
		if (std::string::npos == uri_end)
		{
			uri_end = str_end;
			// то версия протокола HTTP - 0.9
			rp.version = "0.9";
		}
		else // Если окончание найдено
		{
			str_buf[uri_end] = '\0';
			const size_t ver_beg = uri_end + 6; // Пропустить "HTTP/"

			if (ver_beg < str_end)
			{
				// Получить версию протокола HTTP
				rp.version = str_buf.substr(ver_beg, str_end - ver_beg);
			}
		}

		// Поиск именованных параметров запросов (переменных ?)
		const size_t params_pos = str_buf.find('?', delimiter);

		// Сохранить полную ссылку URI (без параметров)
		rp.uri_reference = (std::string::npos == params_pos) ? str_buf.substr(delimiter) : str_buf.substr(delimiter, params_pos - delimiter);

		if (std::string::npos != params_pos)
		{
			// Извлекаем параметры запроса из URI
			if (false == parseIncomingVars(rp.incoming_params, str_buf.substr(params_pos + 1, uri_end) ) )
			{
				// HTTP 400 Bad Request
				return 400;
			}
		}

		// Переход к обработке следующего заголовка
		str_cur = str_end + 2;
		// Поиск конца заголовка
		str_end = str_buf.find("\r\n", str_cur);
		// Установка конца заголовка
		str_buf[str_end] = '\0';

		// Цикл извлечения заголовков запроса
		for (; str_cur != headers_end; str_end = str_buf.find("\r\n", str_cur), str_buf[str_end] = '\0')
		{
			// Поиск разделителя названия заголовка и его значения
			delimiter = str_buf.find(':', str_cur);

			// Если разделитель найден в текущей строке
			if (delimiter < str_end)
			{
				std::string header_name = str_buf.substr(str_cur, delimiter - str_cur);
				std::string header_value = str_buf.substr(delimiter + 1, str_end - delimiter - 1);

				// Удалить лишние пробелы в начале и в конце строки
				Utils::trim(header_value);

				// Сохранить заголовок и его значение
				rp.incoming_headers.emplace(std::move(header_name), std::move(header_value) );
			}

			// Перейти к следующей строке
			str_cur = str_end + 2;
		}

		str_buf.erase(str_buf.begin(), str_buf.begin() + headers_end + 2);

		return 0;
	}

	void Server::runApplication(Socket clientSocket, const ServerApplicationSettings &appSets, struct request_parameters &rp)
	{
		Utils::raw_pair *raw_pair_params = nullptr;
		Utils::raw_pair *raw_pair_headers = nullptr;
		Utils::raw_pair *raw_pair_data = nullptr;
		Utils::raw_fileinfo *raw_fileinfo_files = nullptr;

		Utils::stlToRawPairs(&raw_pair_params, rp.incoming_params);
		Utils::stlToRawPairs(&raw_pair_headers, rp.incoming_headers);
		Utils::stlToRawPairs(&raw_pair_data, rp.incoming_data);
		Utils::filesIncomingToRawFilesInfo(&raw_fileinfo_files, rp.incoming_files);

		server_request request {
			clientSocket.get_handle(),
			rp.method.c_str(),
			rp.uri_reference.c_str(),
			appSets.root_dir.c_str(),
			rp.incoming_params.size(),
			raw_pair_params,
			rp.incoming_headers.size(),
			raw_pair_headers,
			rp.incoming_data.size(),
			raw_pair_data,
			rp.incoming_files.size(),
			raw_fileinfo_files
		};

		server_response response {
			clientSocket.get_handle(), 0, nullptr
		};

		// Попытаться
		try
		{
			// Запустить приложение
			rp.app_exit_code = appSets.application_call(&request, &response);
		}
		catch (...)
		{
			rp.app_exit_code = EXIT_FAILURE;
		}

		if (EXIT_SUCCESS == rp.app_exit_code)
		{
			Utils::rawPairsToStl(rp.outgoing_headers, response.headers, response.headers_count);
		}

		// Очистить заголовки сформированные приложением
		try
		{
			appSets.application_clear(response.headers, response.headers_count);
		}
		catch (...) {}

		Utils::destroyRawPairs(raw_pair_params, rp.incoming_params.size() );
		Utils::destroyRawPairs(raw_pair_headers, rp.incoming_headers.size() );
		Utils::destroyRawPairs(raw_pair_data, rp.incoming_data.size() );
		Utils::destroyRawFilesInfo(raw_fileinfo_files, rp.incoming_files.size() );
	}

	int Server::getRequestData(Socket clientSocket, std::string &str_buf, const ServerApplicationSettings &appSets, struct request_parameters &rp) const
	{
		// Определить вариант данных запроса (заодно проверить, есть ли данные)
		auto it = rp.incoming_headers.find("Content-Type");

		if (rp.incoming_headers.cend() == it)
		{
			return 0;
		}

		// Параметры
		std::unordered_map<std::string, std::string> content_params;

		// Получить значение заголовка
		const std::string &header_value = it->second;

		// Определить, содержит ли тип данных запроса дополнительные параметры
		size_t delimiter = header_value.find(';');

		std::string data_variant_name;	// Название варианта данных запроса

		// Если есть дополнительные параметры - извлекаем их
		if (std::string::npos != delimiter)
		{
			data_variant_name = header_value.substr(0, delimiter);
			Utils::trim(data_variant_name);

			for (size_t str_param_cur = delimiter + 1, str_param_end = 0; std::string::npos != str_param_end; str_param_cur = str_param_end + 1)
			{
				str_param_end = header_value.find(';', str_param_cur);
				delimiter = header_value.find('=', str_param_cur);

				if (delimiter >= str_param_end)
				{
					std::string param_name = header_value.substr(str_param_cur, std::string::npos != str_param_end ? str_param_end - str_param_cur : std::string::npos);
					Utils::trim(param_name);
					content_params.emplace(std::move(param_name), "");
				}
				else
				{
					std::string param_name = header_value.substr(str_param_cur, delimiter - str_param_cur);
					Utils::trim(param_name);

					++delimiter;

					std::string param_value = header_value.substr(delimiter, std::string::npos != str_param_end ? str_param_end - delimiter : std::string::npos);
					Utils::trim(param_value);

					content_params.emplace(std::move(param_name), std::move(param_value) );
				}
			}
		}
		else
		{
			data_variant_name = header_value;
		}

		// Поиск варианта данных по имени типа
		auto variant = this->variants.find(data_variant_name);

		// Если сервер не поддерживает формат полученных данных
		if (this->variants.cend() == variant)
		{
			// HTTP 400 Bad Request
			return 400;
		}

		DataVariantAbstract *data_variant = variant->second;

		// Получить длину запроса в байтах
		size_t data_length = 0;

		auto it_len = rp.incoming_headers.find("Content-Length");

		if (rp.incoming_headers.cend() != it_len)
		{
			data_length = std::strtoull(it_len->second.c_str(), nullptr, 10);
		}

		// Если размер запроса превышает лимит (если лимит был установлен)
		if (data_length > appSets.request_max_size && 0 != appSets.request_max_size)
		{
			// HTTP 413 Request Entity Too Large
			return 413;
		}

		// Сколько осталось получить данных
		size_t left_bytes = 0;

		std::string data_buf;

		if (data_length >= str_buf.length() )
		{
			left_bytes = data_length - str_buf.length();

			data_buf.swap(str_buf);
		}
		else
		{
			data_buf.assign(str_buf.cbegin(), str_buf.cbegin() + data_length);
			str_buf.erase(str_buf.begin(), str_buf.begin() + data_length);
		}

		// Разобрать данные на составляющие
		if (false == data_variant->parse(clientSocket, data_buf, left_bytes, content_params, rp) )
		{
			for (auto &it : rp.incoming_files)
			{
				remove(it.second.getName().c_str() );
			}

			// HTTP 400 Bad Request
			return 400;
		}

		if (false == data_buf.empty() )
		{
			str_buf.swap(data_buf);
		}

		return 0;
	}

	const ServerApplicationSettings *Server::getApplicationSettings(const struct request_parameters &rp) const
	{
		// Получить доменное имя (или адрес) назначения запроса
		auto it_host = rp.incoming_headers.find("Host");

		// Если имя задано - продолжить обработку запроса
		if (rp.incoming_headers.cend() != it_host)
		{
			// Поиск разделителя, за которым помещается номер порта, если указан
			size_t delimiter = it_host->second.find(':');

			// Получить имя (или адрес)
			const std::string host = it_host->second.substr(0, delimiter);

			// Получить номер порта
			const int port = (std::string::npos != delimiter) ? std::strtol(it_host->second.substr(delimiter + 1).c_str(), nullptr, 10) : 80;

			// Поиск настроек приложения по имени
			const ServerApplicationSettings *app_sets = this->apps_tree.find(host);

			// Если приложение найдено
			if (app_sets && app_sets->port == port)
			{
				return app_sets;
			}
		}

		return nullptr;
	}

	void Server::xSendfile(Socket clientSocket, struct request_parameters &rp) const
	{
		auto it_x_sendfile = rp.outgoing_headers.find("X-Sendfile");

		if (rp.outgoing_headers.cend() != it_x_sendfile)
		{
			const std::string connection_header = isConnectionKeepAlive(rp) ? "Connection: Keep-Alive\r\nKeep-Alive: timeout=5; max=" + std::to_string(rp.keep_alive_count) + "\r\n" : "Connection: Close\r\n";

			const bool headers_only = ("head" == rp.method);

			this->transferFile(clientSocket, it_x_sendfile->second, connection_header, headers_only, rp);
		}
	}

	void Server::getConnectionParams(struct request_parameters &rp)
	{
		rp.connection_params = CONNECTION_CLOSED;

		auto it_in_connection = rp.incoming_headers.find("Connection");
		auto it_out_connection = rp.outgoing_headers.find("Connection");

		if (rp.incoming_headers.cend() != it_in_connection && rp.outgoing_headers.cend() != it_out_connection)
		{
			std::locale loc;

			std::string connection_in = it_in_connection->second;
			Utils::tolower(connection_in, loc);

			std::string connection_out = it_out_connection->second;
			Utils::tolower(connection_out, loc);

			auto const incoming_params = Utils::explode(connection_in, ',');

			auto const it = std::find(incoming_params.cbegin(), incoming_params.cend(), connection_out);

			if (incoming_params.cend() != it)
			{
				const std::string &inc = *it;

				if ("keep-alive" == inc)
				{
					--rp.keep_alive_count;

					if (0 < rp.keep_alive_count)
					{
						rp.connection_params |= CONNECTION_KEEP_ALIVE;
					}
				}
				else if ("upgrade" == inc)
				{
					rp.connection_params |= CONNECTION_UPGRADE;
				}
			}
		}
	}

	bool Server::isConnectionKeepAlive(const struct request_parameters &rp)
	{
		return (rp.connection_params & CONNECTION_KEEP_ALIVE) == CONNECTION_KEEP_ALIVE;
	}

	bool Server::isConnectionUpgrade(const struct request_parameters &rp)
	{
		return (rp.connection_params & CONNECTION_UPGRADE) == CONNECTION_UPGRADE;
	}

	/**
	 * Метод для обработки запросов (запускается в отдельном потоке)
	 *	извлекает сокет клиенты из очереди и передаёт его на обслуживание
	 */
	void Server::threadRequestCycle(std::queue<Socket> &sockets) const
	{
		while (true)
		{
			Socket clientSocket;

			this->eventThreadCycle->wait();

			if (false == this->process_flag)
			{
				break;
			}

			this->sockets_queue_mtx.lock();

			if (sockets.size() )
			{
				clientSocket = std::move(sockets.front() );
				sockets.pop();
			}

			if (sockets.empty() )
			{
				this->eventThreadCycle->reset();

				this->eventNotFullQueue->notify();
			}

			this->sockets_queue_mtx.unlock();

			if (clientSocket.is_open() )
			{
				++this->threads_working_count;

				this->threadRequestProc(clientSocket);

				--this->threads_working_count;
			}
		}
	}

	/**
	 * Цикл управления количеством рабочих потоков
	 */
	int Server::cycleQueue(std::queue<Socket> &sockets)
	{
		auto it_option = this->settings.find("threads_max_count");

		size_t threads_max_count = 0;

		if (this->settings.cend() != it_option)
		{
			threads_max_count = std::strtoull(it_option->second.c_str(), nullptr, 10);
		}

		if (0 == threads_max_count)
		{
			threads_max_count = std::thread::hardware_concurrency();

			if (0 == threads_max_count)
			{
				threads_max_count = 1;
			}

			threads_max_count *= 2;
		}

		this->threads_working_count = 0;
		this->eventThreadCycle = new Event(false, true);

		std::function<void(Server *, std::queue<Socket> &)> serverThreadRequestCycle = std::mem_fn(&Server::threadRequestCycle);

		std::vector<std::thread> active_threads;
		active_threads.reserve(threads_max_count);

		// For update applications modules
		do
		{
			if (this->eventUpdateModule->notifed() )
			{
				updateModules();
			}

			// Cycle creation threads applications requests
			do
			{
				while (this->threads_working_count == active_threads.size() && active_threads.size() < threads_max_count && sockets.empty() == false)
				{
					active_threads.emplace_back(serverThreadRequestCycle, this, std::ref(sockets) );
				}

				size_t notify_count = active_threads.size() - this->threads_working_count;

				if (notify_count > sockets.size() )
				{
					notify_count = sockets.size();
				}

				this->eventThreadCycle->notify(notify_count);

				this->eventProcessQueue->wait();
			}
			while (this->process_flag);

			// Data clear

			this->eventThreadCycle->notify();

			if (false == active_threads.empty() )
			{
				// Join threads (wait completion)
				for (auto &th : active_threads)
				{
					th.join();
				}

				active_threads.clear();
			}

			this->eventNotFullQueue->notify();
		}
		while (this->eventUpdateModule->notifed() );

		if (false == this->server_sockets.empty() )
		{
			for (Socket &s : this->server_sockets)
			{
				s.close();
			}

			this->server_sockets.clear();
		}

		delete this->eventThreadCycle;

		return 0;
	}

	Server::Server(): eventNotFullQueue(nullptr), eventProcessQueue(nullptr), eventUpdateModule(nullptr)
	{
		
	}

	void Server::addDataVariant(DataVariantAbstract *dataVariant)
	{
		this->variants.emplace(dataVariant->getName(), dataVariant);
	}

	bool Server::updateModule(Module &module, std::unordered_set<ServerApplicationSettings *> &applications, const size_t moduleIndex)
	{
		std::unordered_set<ServerApplicationSettings *> same;

		for (auto &app : applications)
		{
			if (app->module_index == moduleIndex)
			{
				same.emplace(app);

				try
				{
					if (app->application_final)
					{
						app->application_final();
					}
				}
				catch (...)
				{
					std::cout << "Warning: error when finalizing application '" << app->server_module << "';" << std::endl;
				}

				app->application_call = std::function<int(server_request *, server_response *)>();
				app->application_clear = std::function<void(Utils::raw_pair [], const size_t)>();
				app->application_init = std::function<bool()>();
				app->application_final = std::function<void()>();
			}
		}

		module.close();

		auto app = *(same.cbegin() );

		const std::string &module_name = app->server_module;

	#ifdef WIN32
		std::ifstream src(app->server_module_update, std::ifstream::binary);

		if ( ! src)
		{
			std::cout << "Error: file '" << app->server_module_update << "' cannot be open;" << std::endl;
			return false;
		}

		std::ofstream dst(module_name, std::ofstream::binary | std::ofstream::trunc);

		if ( ! dst)
		{
			std::cout << "Error: file '" << module_name << "' cannot be open;" << std::endl;
			return false;
		}

		// Copy (rewrite) file
		dst << src.rdbuf();

		src.close();
		dst.close();

		// Open updated module
		module.open(module_name);

	#elif POSIX
		// HACK: for posix system - load new version shared library

		size_t dir_pos = module_name.rfind('/');
		size_t ext_pos = module_name.rfind('.');

		std::string module_name_temp;

		if (std::string::npos != ext_pos && (std::string::npos == dir_pos || dir_pos < ext_pos) )
		{
			module_name_temp = module_name.substr(0, ext_pos) + '-' + Utils::getUniqueName() + module_name.substr(ext_pos);
		}
		else
		{
			module_name_temp = module_name + '-' + Utils::getUniqueName();
		}

		std::ifstream src(app->server_module_update, std::ifstream::binary);

		if ( ! src)
		{
			std::cout << "Error: file '" << app->server_module_update << "' cannot be open;" << std::endl;
			return false;
		}

		std::ofstream dst(module_name_temp, std::ofstream::binary | std::ofstream::trunc);

		if ( ! dst)
		{
			std::cout << "Error: file '" << module_name << "' cannot be open;" << std::endl;
			return false;
		}

		// Copy (rewrite) file
		dst << src.rdbuf();

		src.close();
		dst.close();

		// Open updated module
		module.open(module_name_temp);

		if (0 != remove(module_name.c_str() ) )
		{
			std::cout << "Error: file '" << module_name << "' cannot be removed;" << std::endl;
			return false;
		}

		if (0 != rename(module_name_temp.c_str(), module_name.c_str() ) )
		{
			std::cout << "Error: file '" << module_name_temp << "' cannot be renamed;" << std::endl;
			return false;
		}
	#else
		#error "Undefine platform"
	#endif

		if (false == module.is_open() )
		{
			std::cout << "Error: application module '" << module_name << "' cannot be open;" << std::endl;
			return false;
		}

		void *(*addr)(void *) = nullptr;

		if (false == module.find("application_call", &addr) )
		{
			std::cout << "Error: function 'application_call' not found in module '" << module_name << "';" << std::endl;
			return false;
		}

		std::function<int(server_request *, server_response *)> app_call = reinterpret_cast<int(*)(server_request *, server_response *)>(addr);

		if ( ! app_call)
		{
			std::cout << "Error: invalid function 'application_call' in module '" << module_name << "';" << std::endl;
			return false;
		}

		if (false == module.find("application_clear", &addr) )
		{
			std::cout << "Error: function 'application_clear' not found in module '" << module_name << "';" << std::endl;
			return false;
		}

		std::function<void(Utils::raw_pair [], const size_t)> app_clear = reinterpret_cast<void(*)(Utils::raw_pair [], const size_t)>(addr);

		std::function<bool()> app_init = std::function<bool()>();

		if (module.find("application_init", &addr) )
		{
			app_init = reinterpret_cast<bool(*)()>(addr);
		}

		std::function<void()> app_final = std::function<void()>();

		if (module.find("application_final", &addr) )
		{
			app_final = reinterpret_cast<void(*)()>(addr);
		}

		for (auto &app : same)
		{
			app->application_call = app_call;
			app->application_clear = app_clear;
			app->application_init = app_init;
			app->application_final = app_final;

			try
			{
				if (app->application_init)
				{
					app->application_init();
				}
			}
			catch (...)
			{
				std::cout << "Warning: error when initializing application '" << module_name << "';" << std::endl;
			}
		}

		return true;
	}

	void Server::updateModules()
	{
		// Applications settings list
		std::unordered_set<ServerApplicationSettings *> applications;
		// Get full applications settings list
		this->apps_tree.collectApplicationSettings(applications);

		std::unordered_set<size_t> updated;

		for (auto &app : applications)
		{
			const size_t module_index = app->module_index;

			// If module is not updated (not checked)
			if (updated.end() == updated.find(module_index) )
			{
				if (false == app->server_module_update.empty() && app->server_module_update != app->server_module)
				{
					size_t module_size_new = 0;
					time_t module_time_new = 0;

					if (System::getFileSizeAndTimeGmt(app->server_module_update, &module_size_new, &module_time_new) )
					{
						size_t module_size_cur = 0;
						time_t module_time_cur = 0;

						Module &module = this->modules[module_index];

						if (System::getFileSizeAndTimeGmt(app->server_module, &module_size_cur, &module_time_cur) )
						{
							if (module_size_cur != module_size_new || module_time_cur < module_time_new)
							{
								this->updateModule(module, applications, module_index);
							}
						}
					}
				}

				updated.emplace(module_index);
			}
		}

		std::cout << "Notice: applications modules have been updated;" << std::endl;

		this->process_flag = true;
		this->eventUpdateModule->reset();
	}

	bool Server::init()
	{
		ConfigParser conf_parser;

		if (Socket::Startup() && conf_parser.loadConfig("main.conf", this->settings, this->mimes_types, this->modules, this->apps_tree) )
		{
			this->addDataVariant(new DataVariantFormUrlencoded() );
			this->addDataVariant(new DataVariantMultipartFormData() );
			this->addDataVariant(new DataVariantTextPlain() );

			return true;
		}

		return false;
	}

	void Server::clear()
	{
		Socket::Cleanup();

		if (this->eventNotFullQueue)
		{
			delete this->eventNotFullQueue;
			this->eventNotFullQueue = nullptr;
		}

		if (this->eventProcessQueue)
		{
			delete this->eventProcessQueue;
			this->eventProcessQueue = nullptr;
		}

		if (this->eventUpdateModule)
		{
			delete this->eventUpdateModule;
			this->eventUpdateModule = nullptr;
		}

		if (false == this->variants.empty() )
		{
			for (auto &variant : this->variants)
			{
				delete variant.second;
			}

			this->variants.clear();
		}

		if (false == this->apps_tree.empty() )
		{
			std::unordered_set<ServerApplicationSettings *> applications;
			this->apps_tree.collectApplicationSettings(applications);

			for (auto &app : applications)
			{
				try
				{
					if (app->application_final)
					{
						app->application_final();
					}
				}
				catch (...)
				{
					std::cout << "Warning: error when finalizing application '" << app->server_module << "';" << std::endl;
				}

				delete app;
			}

			applications.clear();
			this->apps_tree.clear();
		}

		if (false == this->modules.empty() )
		{
			for (auto &module : this->modules)
			{
				module.close();
			}

			this->modules.empty();
		}

		if (false == this->settings.empty() )
		{
			this->settings.clear();
		}
	}

	int Server::run()
	{
		if (false == init() )
		{
			return 1;
		}

		// Applications settings list
		std::unordered_set<ServerApplicationSettings *> applications;
		// Get full applications settings list
		this->apps_tree.collectApplicationSettings(applications);

		// Bind ports set
		std::unordered_set<int> ports;

		// Open applications sockets
		for (auto &app : applications)
		{
			const int port = app->port;

			// Only unique ports
            if (ports.cend() == ports.find(port) )
			{
				Socket sock;

				if (~0 != sock.open() )
				{
					if (~0 != sock.bind(port) )
					{
						if (0 == sock.listen() )
						{
							sock.nonblock(true);

							this->server_sockets.emplace_back(std::move(sock) );

							ports.emplace(port);
						}
						else
						{
							std::cout << "Error: cannot listen socket " << port << "; errno " << errno << ";" << std::endl;
						}
					}
					else
					{
						std::cout << "Error: cannot bind socket " << port << "; errno " << errno << ";" << std::endl;
					}
				}
				else
				{
					std::cout << "Error: cannot open socket; errno " << errno << ";" << std::endl;
				}
			}
		}

		if (this->server_sockets.empty() )
		{
			std::cout << "Error: do not open any socket;" << std::endl;
			return 2;
		}

		SocketList sockets_list;

		sockets_list.create(this->server_sockets.size() );

		for (auto &sock : this->server_sockets)
		{
			sockets_list.addSocket(sock);
		}

		std::cout << "Log: start server cycle;" << std::endl << std::endl;

		const size_t queue_max_length = 1024;
		this->eventNotFullQueue = new Event(true, true);
		this->eventProcessQueue = new Event();
		this->eventUpdateModule = new Event(false, true);

		std::queue<Socket> sockets;

		this->process_flag = true;

		std::function<int(Server *, std::queue<Socket> &)> serverCycleQueue = std::mem_fn(&Server::cycleQueue);

		std::vector<Socket> client_sockets;

		std::thread threadQueue(serverCycleQueue, this, std::ref(sockets) );

		// Cycle receiving new connections
		do
		{
			if (sockets_list.accept(client_sockets) )
			{
				this->sockets_queue_mtx.lock();

				for (Socket &sock : client_sockets)
				{
					if (sock.is_open() )
					{
						sock.nonblock(true);
						sock.tcp_nodelay(true);
						sockets.emplace(std::move(sock) );
					}
				}

				this->sockets_queue_mtx.unlock();

				this->eventProcessQueue->notify();

				if (sockets.size() >= queue_max_length)
				{
					this->eventNotFullQueue->reset();
				}

				client_sockets.clear();

				this->eventNotFullQueue->wait();
			}
		}
		while (this->process_flag || this->eventUpdateModule->notifed() );

		this->eventProcessQueue->notify();

		threadQueue.join();

		sockets_list.destroy();

		clear();

		std::cout << "Log: final server cycle;" << std::endl;

		return EXIT_SUCCESS;
	}

	void Server::stopProcess()
	{
		this->process_flag = false;

		if (this->eventNotFullQueue)
		{
			this->eventNotFullQueue->notify();
		}

		this->setProcessQueue();
	}

	int Server::command_start(const int argc, const char *argv[])
	{
		const std::string pid_file_name = "httpserver.pid";

		// TODO:
		// Проверить, существует ли файл и открыт ли он уже на запись - значит сервер уже запущен - нужно завершить текущий (this) процесс

		// Создать файл для хранения идентификатора процесса
		std::ofstream file(pid_file_name, std::ofstream::trunc);

		// Если создать не удалось
		if (false == file.good() || false == file.is_open() )
		{
			file.close();
			return 7;
		}

		// Записать идентификатор текущего процесса в файл
		file << std::to_string(System::getProcessId() ) << std::flush;

		int code = EXIT_FAILURE;

		do
		{
			this->process_flag = false;
			this->restart_flag = false;

			code = this->run();
		}
		while (this->process_flag || this->restart_flag);

		file.close();

		remove(pid_file_name.c_str() );

		return code;
	}

	System::native_processid_type Server::getPidFromFile() const
	{
		System::native_processid_type pid = 0;

		std::ifstream file("httpserver.pid");

		if (file.good() && file.is_open() )
		{
			char str_pid[32] = {0};

			file.get(str_pid, sizeof(str_pid) );

			if (file.gcount() )
			{
				pid = std::strtoull(str_pid, nullptr, 10);
			}
		}

		file.close();

		return pid;
	}

	int Server::command_help(const int argc, const char *argv[]) const
	{
		std::cout << "Available arguments:" << std::endl
			<< std::setw(4) << ' ' << "--start" << "\t\t" << "Start http server" << std::endl
			<< std::setw(4) << ' ' << "--restart" << "\t\t" << "Restart http server" << std::endl
			<< std::setw(4) << ' ' << "--update_module" << "\t" << "Update applications modules" << std::endl
			<< std::setw(4) << ' ' << "--kill" << "\t\t" << "Shutdown http server" << std::endl
			<< std::setw(4) << ' ' << "--help" << "\t\t" << "This help" << std::endl << std::endl;

		return EXIT_SUCCESS;
	}

	int Server::command_restart(const int argc, const char *argv[]) const
	{
		const System::native_processid_type pid = getPidFromFile();

		if (1 < pid && System::sendSignal(pid, SIGUSR1) )
		{
			return EXIT_SUCCESS;
		}

		return EXIT_FAILURE;
	}

	int Server::command_terminate(const int argc, const char *argv[]) const
	{
		const System::native_processid_type pid = getPidFromFile();

		if (1 < pid && System::sendSignal(pid, SIGTERM) )
		{
			return EXIT_SUCCESS;
		}

		return EXIT_FAILURE;
	}

	int Server::command_update_module(const int argc, const char *argv[]) const
	{
		const System::native_processid_type pid = getPidFromFile();

		if (1 < pid && System::sendSignal(pid, SIGUSR2) )
		{
			return EXIT_SUCCESS;
		}

		return EXIT_FAILURE;
	}
};