
#include "Server.h"
#include "Utils.h"
#include "System.h"

#include "DataVariantFormUrlencoded.h"
#include "DataVariantMultipartFormData.h"
#include "DataVariantTextPlain.h"
#include "FileIncoming.h"
#include "ServerRequest.h"
#include "ServerResponse.h"

#include <iostream>
#include <map>
#include <thread>
#include <fstream>
#include <cstring>

namespace HttpServer
{
	/**
	 * Передача файла (или его части)
	 */
	int Server::transferFile(const Socket &clientSocket, const std::chrono::milliseconds &timeout, const std::string &fileName, const std::unordered_map<std::string, std::string> &inHeaders, const std::map<std::string, std::string> &outHeaders, const std::string &connectionHeader) const
	{
		// Get current time in GMT
		const std::string date_header = "Date: " + Utils::getDatetimeStringValue() + "\r\n";

		time_t file_time;
		size_t file_size;

		// Получить размер файла и дату последнего изменения
		if (false == System::getFileSizeAndTimeGmt(fileName, &file_size, &file_time) )
		{
			// HTTP 404
			std::string headers("HTTP/1.1 404 Not Found\r\n");
			headers += connectionHeader + date_header;
			headers += "\r\n";

			clientSocket.nonblock_send(headers, timeout);

			return 0;
		}

		// Check for If-Modified header
		auto it_modified = inHeaders.find("If-Modified-Since");

		// Если найден заголовок проверки изменения файла (проверить, изменялся ли файл)
		if (inHeaders.end() != it_modified)
		{
			const time_t time_in_request = Utils::stringTimeToTimestamp(it_modified->second);

			if (file_time == time_in_request)
			{
				// HTTP 304
				std::string headers("HTTP/1.1 304 Not Modified\r\n");
				headers += connectionHeader + date_header;
				headers += "\r\n";

				clientSocket.nonblock_send(headers, timeout);

				return 0;
			}
		}

		// TODO: Range check

		// TODO: file transfer
		std::ifstream file(fileName);

		if ( ! file)
		{
			file.close();

			// HTTP 500
			std::string headers("HTTP/1.1 500 Internal Server Error\r\n");
			headers += connectionHeader + date_header;
			headers += "\r\n";

			clientSocket.nonblock_send(headers, timeout);

			return 0;
		}

		// TODO: Get file mime type (by file extenition)
		const size_t ext_pos = fileName.rfind('.');
		const std::string file_ext = std::string::npos != ext_pos ? fileName.substr(ext_pos + 1) : "";

		auto it_mime = mimes_types.find(file_ext);

		std::string file_mime_type = mimes_types.end() != it_mime ? it_mime->second : "application/octet-stream";
	//	std::string file_mime_type("text/html");

		std::string headers("HTTP/1.1 200 OK\r\n");
		headers += "Content-Type: " + file_mime_type + "\r\n";
		headers += "Content-Length: " + std::to_string(file_size) + "\r\n";
		headers += "Last-Modified: " + Utils::getDatetimeStringValue(file_time, true) + "\r\n";
		headers += connectionHeader + date_header;
		headers += "\r\n";

		// Отправить заголовки
		if (std::numeric_limits<size_t>::max() == clientSocket.nonblock_send(headers, timeout) )
		{
			file.close();

			return 0;
		}

		// Отправить файл
		if (file_size)
		{
			std::vector<std::string::value_type> buf(file_size < 512 * 1024 ? file_size : 512 * 1024);

			size_t send_size;

			do
			{
				file.read(reinterpret_cast<char *>(buf.data() ), buf.size() );
				send_size = clientSocket.nonblock_send(buf, file.gcount(), timeout);
			}
			while (false == file.eof() && std::numeric_limits<size_t>::max() != send_size);
		}

		file.close();

		return 0;
	}

	/**
	 * Парсинг переданных параметров (URI)
	 */
	void Server::parseIncomingVars(std::unordered_multimap<std::string, std::string> &map, const std::string &str, const size_t start, const size_t end) const
	{
		if (str.length() > start)
		{
			size_t var_pos = start;
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
						var_value = str.substr(delimiter, end - delimiter);
					}
					else // Если не последний параметр
					{
						var_value = str.substr(delimiter, var_end - delimiter);
					}

					// Сохранить параметр и значение
					map.emplace(var_name, var_value);

					if (std::string::npos != var_end)
					{
						++var_end;
					}
				}
			}
		}
	}

	/**
	 * Метод для обработки запроса (запускается в отдельном потоке)
	 */
	int Server::threadRequestProc(Socket &clientSocket)
	{
		int app_exit_code;

		bool connection_upgrade = false;
		bool connection_keep_alive;

		do
		{
			app_exit_code = EXIT_FAILURE;

			const size_t buf_len = 4096;
			std::vector<std::string::value_type> buf(buf_len);
			std::string str_buf;

			std::unordered_map<std::string, std::string> incoming_headers;
			std::unordered_multimap<std::string, std::string> incoming_params;
			std::unordered_multimap<std::string, std::string> incoming_data;
			std::unordered_multimap<std::string, FileIncoming> incoming_files;

			std::map<std::string, std::string> outgoing_headers;

			std::string method;
			std::string version;
			std::string uri_reference;

			// Получить данные запроса от клиента
			std::chrono::milliseconds timeout(5000);
			const size_t recv_len = clientSocket.nonblock_recv(buf, timeout);

			if (std::numeric_limits<size_t>::max() == recv_len)
			{
			#ifdef WIN32
				std::cout << "Error: " << WSAGetLastError() << std::endl;
			#elif POSIX
				std::cout << "Error: " << errno << std::endl;
			#endif

				// TODO: exit thread
				break;
			}
			else if (recv_len) // Если данные были получены
			{
				str_buf.assign(buf.cbegin(), buf.cbegin() + recv_len);

			//	std::cout << str_buf << std::endl << typeid(*this).name() << "\n" << __PRETTY_FUNCTION__ << "\n" << __FUNCTION__ << "\n" << __LINE__  << "\n---Thank you for request!---\n\n";

				// Поиск конца заголовков (пустая строка)
				size_t headers_end = str_buf.find("\r\n\r\n");

				// Если найден конец заголовков
				if (std::string::npos != headers_end)
				{
					headers_end += 2;

					size_t str_cur = 0;
					// Поиск конца первого заголовка
					size_t str_end = str_buf.find("\r\n");

					// Если не найден конец заголовка
					if (std::string::npos == str_end)
					{
						// TODO: exit thread
						break;
					}

					// Установка конца строки (для поиска)
					str_buf[str_end] = '\0';

					// Разделить метод запроса и параметры запроса
					size_t delimiter = str_buf.find(' ', str_cur);

					// Получить метод запроса (GET, POST, PUT, DELETE, ...)
					method = str_buf.substr(str_cur, delimiter - str_cur);
					// Сохранить метод и параметры запроса
					incoming_headers[method] = str_buf.substr(delimiter + 1, str_end - delimiter - 1);

					delimiter += 1;
					// Найти окончание URI
					size_t uri_end = str_buf.find(' ', delimiter);

					// Если окончание не найдено
					if (std::string::npos == uri_end)
					{
						uri_end = str_end;
						// то версия протокола HTTP - 0.9
						version = "0.9";
					}
					else // Если окончание найдено
					{
						str_buf[uri_end] = '\0';
						const size_t ver_beg = uri_end + 6; // Пропустить "HTTP/"

						if (ver_beg < str_end)
						{
							// Получить версию протокола HTTP
							version = str_buf.substr(ver_beg, str_end - ver_beg);
						}
					}

					// Поиск именованных параметров запросов (переменных ?)
					size_t params_pos = str_buf.find('?', delimiter);

					// Сохранить полную ссылку URI (с параметрами)
					uri_reference = (std::string::npos == params_pos) ? str_buf.substr(delimiter) : str_buf.substr(delimiter, params_pos - delimiter);

					if (std::string::npos != params_pos)
					{
						// Извлекаем параметры запроса из URI
						parseIncomingVars(incoming_params, str_buf, params_pos + 1, uri_end);
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
							incoming_headers.emplace(header_name, header_value);
						}

						// Перейти к следующей строке
						str_cur = str_end + 2;
					}

					// Получить доменное имя (или адрес) назначения запроса
					auto it_host = incoming_headers.find("Host");

					// Если имя задано - продолжить обработку запроса
					if (incoming_headers.end() != it_host)
					{
						// Поиск разделителя, за которым помещается номер порта (сокета), если указан
						size_t delimiter = it_host->second.find(':');

						// Получить имя (или адрес)
						std::string host = it_host->second.substr(0, delimiter);

					/*	TODO: port
						if (std::string::npos != delimiter)
						{
							host.erase(delimiter);
						}*/

						// Поиск настроек приложения по имени
						ServerApplicationSettings *app_sets = apps_tree.find(host);

						// Если приложение найдено
						if (app_sets)
						{
							// Определить вариант данных запроса (заодно проверить, есть ли данные)
							auto it = incoming_headers.find("Content-Type");

							if (incoming_headers.end() != it)
							{
								// Параметры
								std::unordered_map<std::string, std::string> content_params;

								// Получить значение заголовка
								std::string &header_value = it->second;

								// Определить, содержит ли тип данных запроса дополнительные параметры
								delimiter = header_value.find(';');

								std::string data_variant_name;	// Название варианта данных запроса

								// Если есть дополнительные параметры - извлекаем их
								if (std::string::npos != delimiter)
								{
									data_variant_name = header_value.substr(0, delimiter);
									Utils::trim(data_variant_name);

									for (size_t str_param_cur = delimiter + 1, str_param_end; std::string::npos != str_param_cur; str_param_cur = str_param_end)
									{
										str_param_end = header_value.find(';', str_param_cur);
										delimiter = header_value.find('=', str_param_cur);

										if (std::string::npos == delimiter || delimiter > str_param_end)
										{
											std::string param_name = (std::string::npos == str_param_end) ? header_value.substr(str_param_cur) : header_value.substr(str_param_cur, str_param_end - str_param_cur);
											Utils::trim(param_name);
											content_params.emplace(param_name, "");
										}
										else
										{
											std::string param_name = header_value.substr(str_param_cur, delimiter - str_param_cur);
											Utils::trim(param_name);

											++delimiter;

											std::string param_value = (std::string::npos == str_param_end) ? header_value.substr(delimiter) : header_value.substr(delimiter, str_param_end - delimiter);
											Utils::trim(param_value);

											content_params.emplace(param_name, param_value);
										}

										if (std::string::npos != str_param_end)
										{
											++str_param_end;
										}
									}
								}
								else
								{
									data_variant_name = header_value;
								}

								// Поиск варианта данных по имени типа
								auto variant = variants.find(data_variant_name);

								// Если сервер поддерживает формат полученных данных
								if (variants.end() != variant)
								{
									DataVariantAbstract *data_variant = variant->second;

									// Получить длину запроса в байтах
									size_t data_length = 0;

									auto it_len = incoming_headers.find("Content-Length");

									if (incoming_headers.end() != it_len)
									{
										data_length = std::stoull(it_len->second);
									}

									// Если размер запроса не превышает лимит (если лимит был установлен)
									if (data_length <= app_sets->request_max_size || 0 == app_sets->request_max_size)
									{
										// Сколько осталось получить данных
										size_t left_bytes = 0;

										if (data_length)
										{
											left_bytes = data_length - (recv_len - (headers_end + 2) );
										}

										// Разобрать данные на составляющие
										if (false == data_variant->parse(clientSocket, timeout, str_buf.substr(headers_end + 2), left_bytes, content_params, incoming_data, incoming_files) )
										{
											// TODO: HTTP 400 Bad Request

											for (auto it : incoming_files)
											{
												remove(it.second.getName().c_str() );
											}

											break;
										}
									}
									else
									{
										// TODO: HTTP 413 Request Entity Too Large
									}
								}
								else
								{
									// TODO: HTTP 400 Bad Request
								}
							}

							Utils::raw_pair *raw_pair_params = nullptr;
							Utils::raw_pair *raw_pair_headers = nullptr;
							Utils::raw_pair *raw_pair_data = nullptr;
							Utils::raw_fileinfo *raw_fileinfo_files = nullptr;

							stlUnorderedMultimapToRawPairs(&raw_pair_params, incoming_params);
							stlUnorderedMapToRawPairs(&raw_pair_headers, incoming_headers);
							stlUnorderedMultimapToRawPairs(&raw_pair_data, incoming_data);
							filesIncomingToRawFilesInfo(&raw_fileinfo_files, incoming_files);

							server_request request {
								clientSocket.get_handle(),
								method.c_str(),
								uri_reference.c_str(),
								app_sets->root_dir.c_str(),
								incoming_params.size(),
								raw_pair_params,
								incoming_headers.size(),
								raw_pair_headers,
								incoming_data.size(),
								raw_pair_data,
								incoming_files.size(),
								raw_fileinfo_files
							};

							server_response response {
								clientSocket.get_handle(), 0, nullptr
							};

							// Попытаться
							try
							{
								// Запустить приложение
								app_exit_code = app_sets->application_call(&request, &response);
							}
							catch (...)
							{
								app_exit_code = EXIT_FAILURE;
							}

							if (EXIT_SUCCESS == app_exit_code)
							{
								rawPairsToStlMap(outgoing_headers, response.headers, response.headers_count);
							}

							if (response.headers_count && response.headers)
							{
								destroyRawPairs(response.headers, response.headers_count);
							}

							destroyRawPairs(raw_pair_params, incoming_params.size() );
							destroyRawPairs(raw_pair_headers, incoming_headers.size() );
							destroyRawPairs(raw_pair_data, incoming_data.size() );
							destroyRawFilesInfo(raw_fileinfo_files, incoming_files.size() );
						}
						else
						{
							// TODO: HTTP 404 Not Found
						}
					}
					else
					{
						// TODO: HTTP 400 Bad Request
					}
				}
				else
				{
					// TODO: HTTP 400 Bad Request
				}
			}
			else // Если запрос пустой
			{
				// TODO: HTTP 400 Bad Request
				break;
			}

			for (auto it : incoming_files)
			{
				remove(it.second.getName().c_str() );
			}

			connection_keep_alive = false;

			if (EXIT_SUCCESS == app_exit_code)
			{
				auto it_in_connection = incoming_headers.find("Connection");
				auto it_out_connection = outgoing_headers.find("Connection");

				if (incoming_headers.end() != it_in_connection && outgoing_headers.end() != it_out_connection)
				{
					std::locale loc;

					std::string connection_in = it_in_connection->second;
					Utils::tolower(connection_in, loc);

					std::string connection_out = it_out_connection->second;
					Utils::tolower(connection_out, loc);

					if ("keep-alive" == connection_in)
					{
						if ("keep-alive" == connection_out)
						{
							connection_keep_alive = true;
						}
					}
					else if ("upgrade" == connection_in)
					{
						if ("upgrade" == connection_out)
						{
							connection_upgrade = true;
						}
					}
				}

				auto it_x_sendfile = outgoing_headers.find("X-Sendfile");

				if (outgoing_headers.end() != it_x_sendfile)
				{
					const std::string connection_header = connection_keep_alive ? "Connection: keep-alive\r\n" : "Connection: close\r\n";

					transferFile(clientSocket, timeout, it_x_sendfile->second, incoming_headers, outgoing_headers, connection_header);
				}
			}
		}
		while (connection_keep_alive);

		if (false == connection_upgrade)
		{
			clientSocket.shutdown();
			clientSocket.close();
		}

		return app_exit_code;
	}

	int Server::cycleQueue(std::queue<std::shared_ptr<Socket> > &sockets)
	{
		auto it_option = settings.find("threads_max_count");

		size_t threads_max_count = 0;

		if (settings.end() != it_option)
		{
			threads_max_count = std::stoul(it_option->second);
		}

		if (0 == threads_max_count)
		{
			threads_max_count = System::getProcessorsCount();
		}

		std::function<int(Server &, Socket &)> serverThreadRequestProc = std::mem_fn(&Server::threadRequestProc);

		std::vector<std::thread> active_threads;
		std::vector<std::shared_ptr<Socket> > active_sockets;

		active_threads.reserve(threads_max_count);
		active_sockets.reserve(threads_max_count);

		while (process_flag)
		{
			eventProcessQueue->wait();

			if (threads_max_count <= active_threads.size() )
			{
				size_t i = 0;

				while (false == System::isDoneThread(active_threads[i].native_handle() ) )
				{
					if (++i == active_threads.size() )
					{
						std::this_thread::yield();
						i = 0;
					}
				}
			}

			for (size_t i = 0; i != active_threads.size();)
			{
				auto th = active_threads.begin() + i;

				if (System::isDoneThread(th->native_handle() ) )
				{
					th->join();
					active_threads.erase(th);
					active_sockets.erase(active_sockets.begin() + i);
				}
				else
				{
					++i;
				}
			}

			while (active_threads.size() <= threads_max_count && sockets.empty() == false)
			{
				std::shared_ptr<Socket> client_socket = sockets.front();
				sockets.pop();

				active_threads.emplace_back(serverThreadRequestProc, std::ref(*this), std::ref(*client_socket) );
				active_sockets.emplace_back(client_socket);
			}

			if (false == eventNotFullQueue->notifed() )
			{
				eventNotFullQueue->notify();
			}
		}

		// Очистка данных

		if (false == active_threads.empty() )
		{
			// Присоединить потоки (подождать их завершения)
			for (auto &th : active_threads)
			{
				th.join();
			}

			active_threads.clear();
		}

		if (false == active_sockets.empty() )
		{
			for (std::shared_ptr<Socket> &sock : active_sockets)
			{
				sock->shutdown();
				sock->close();
			}

			active_sockets.clear();
		}

		return 0;
	}

	Server::Server(): eventNotFullQueue(nullptr), eventProcessQueue(nullptr)
	{
		
	}

	void Server::addDataVariant(DataVariantAbstract *postVariant)
	{
		variants.emplace(postVariant->getName(), postVariant);
	}

	bool Server::includeConfigFile(const std::string &fileName, std::string &strBuf, const std::size_t offset = 0)
	{
		std::ifstream file(fileName);

		if (false == file.good() || false == file.is_open() )
		{
			file.close();

			std::cout << fileName << " - cannot be open;" << std::endl;

			return false;
		}

		file.seekg(0, std::ifstream::end);
		std::streamsize file_size = file.tellg();
		file.seekg(0, std::ifstream::beg);

		std::streamsize file_size_max = 2048 * 1024;

		if (file_size_max < file_size)
		{
			file.close();

			std::cout << fileName << " - is too large; max include file size = " << file_size_max << " bytes;" << std::endl;

			return false;
		}

		if (file_size)
		{
			std::vector<std::string::value_type> buf(file_size);
			file.read(reinterpret_cast<char *>(buf.data() ), file_size);

			strBuf.insert(strBuf.begin() + offset, buf.cbegin(), buf.cend() );
		}

		file.close();

		return true;
	}

	bool Server::loadConfig()
	{
		std::string file_name("main.conf");
		std::string str_buf;

		if (false == includeConfigFile(file_name, str_buf) )
		{
			return false;
		}

		std::vector<std::unordered_map<std::string, std::string> > applications;

		std::string whitespace(" \t\n\v\f\r");

		size_t cur_pos = 0;
		size_t end_pos = str_buf.find(';', cur_pos);
		size_t block_pos = 0;

		while (std::string::npos != end_pos)
		{
			block_pos = str_buf.find('{', cur_pos);

			if (end_pos < block_pos)
			{
				cur_pos = str_buf.find_first_not_of(whitespace, cur_pos);
				size_t delimiter = str_buf.find_first_of(whitespace, cur_pos);

				if (delimiter < end_pos)
				{
					std::string param_name = str_buf.substr(cur_pos, delimiter - cur_pos);
					Utils::trim(param_name);

					std::string param_value = str_buf.substr(delimiter + 1, end_pos - delimiter - 1);
					Utils::trim(param_value);

					if ("include" == param_name)
					{
						includeConfigFile(param_value, str_buf, end_pos + 1);
					}
					else
					{
						settings.emplace(param_name, param_value);
					}
				}

				cur_pos = end_pos + 1;
			}
			else if (std::string::npos != block_pos)
			{
				std::string block_type_name = str_buf.substr(cur_pos, block_pos - cur_pos);
				Utils::trim(block_type_name);

				cur_pos = block_pos + 1;

				if ("server" == block_type_name)
				{
					std::unordered_map<std::string, std::string> app;

					end_pos = str_buf.find(';', cur_pos);
					size_t block_end = str_buf.find('}', cur_pos);

					while (block_end > end_pos)
					{
						cur_pos = str_buf.find_first_not_of(whitespace, cur_pos);
						size_t delimiter = str_buf.find_first_of(whitespace, cur_pos);

						if (delimiter < end_pos)
						{
							std::string param_name = str_buf.substr(cur_pos, delimiter - cur_pos);
							Utils::trim(param_name);

							std::string param_value = str_buf.substr(delimiter + 1, end_pos - delimiter - 1);
							Utils::trim(param_value);

							if ("include" == param_name)
							{
								cur_pos = end_pos + 1;
								includeConfigFile(param_value, str_buf, cur_pos);
								block_end = str_buf.find('}', cur_pos);
							}
							else
							{
								app.emplace(param_name, param_value);
							}
						}

						cur_pos = end_pos + 1;

						end_pos = str_buf.find(';', cur_pos);
					}

					applications.emplace_back(app);

					cur_pos = block_end + 1;
				}
				else
				{
					std::cout << block_type_name << " - unknown block type;" << std::endl;
				}
			}

			end_pos = str_buf.find(';', cur_pos);
		}

		mimes_types["html"] = "text/html";
		mimes_types["js"] = "text/javascript";
		mimes_types["css"] = "text/css";
		mimes_types["png"] = "image/png";

		if (applications.size() )
		{
			auto it_default_temp_dir = settings.find("default_temp_dir");

			const std::string default_temp_dir = settings.end() != it_default_temp_dir ? it_default_temp_dir->second : System::getTempDir();

			auto it_default_request_max_size = settings.find("request_max_size");

			const size_t default_request_max_size = settings.end() != it_default_request_max_size ? std::stoull(it_default_request_max_size->second) : 0;

			std::vector<std::string> names;

			for (auto &app : applications)
			{
				auto it_name = app.find("server_name");

				if (app.end() != it_name)
				{
					if (false == names.empty() )
					{
						names.clear();
					}

					std::string &app_name = it_name->second;
				
					size_t delimiter = app_name.find_first_of(whitespace);

					if (delimiter)
					{
						size_t cur_pos = 0;

						while (std::string::npos != delimiter)
						{
							std::string name = app_name.substr(cur_pos, delimiter - cur_pos);
							names.emplace_back(name);
							cur_pos = app_name.find_first_not_of(whitespace, delimiter + 1);
							delimiter = app_name.find_first_of(whitespace, cur_pos);
						}

						std::string name = app_name.substr(cur_pos);
						names.emplace_back(name);
					}

					auto it_root_dir = app.find("root_dir");

					if (app.end() != it_root_dir && it_root_dir->second.length() )
					{
						auto it_module = app.find("server_module");

						if (app.end() != it_module)
						{
							Module module(it_module->second);

							if (module.is_open() )
							{
								bool is_exists = false;

								for (size_t i = 0; i < modules.size(); ++i)
								{
									if (modules[i] == module)
									{
										is_exists = true;
										break;
									}
								}

								if (false == is_exists)
								{
									modules.push_back(module);
								}

								void *addr = nullptr;

								if (module.find("application_call", &addr) )
								{
									std::function<int(server_request *, server_response *)> app_call = reinterpret_cast<int(*)(server_request *, server_response *)>(addr);

									if (app_call)
									{
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

										bool success = true;

										try
										{
											if (app_init)
											{
												success = app_init();
											}
										}
										catch (...)
										{
											success = false;
										}

										if (success)
										{
											auto it_temp_dir = app.find("temp_dir");

											const std::string temp_dir = app.end() != it_temp_dir ? it_temp_dir->second : default_temp_dir;

											auto it_request_max_size = app.find("request_max_size");

											const size_t request_max_size = app.end() != it_request_max_size ? std::stoull(it_request_max_size->second) : default_request_max_size;

											// Если путь к директории заканчивается слешем, то убираем его
											if ('/' == it_root_dir->second.back() )
											{
												it_root_dir->second.assign(it_root_dir->second.cbegin(), it_root_dir->second.cend() - 1);
											}

											ServerApplicationSettings *sets = new ServerApplicationSettings {
												it_root_dir->second,
												temp_dir,
												request_max_size,
												app_call,
												app_init,
												app_final
											};

											if (names.empty() )
											{
												apps_tree.addApplication(app_name, sets);
											}
											else
											{
												for (size_t i = 0; i < names.size(); ++i)
												{
													apps_tree.addApplication(names[i], sets);
												}
											}
										} // end success
									} // end if app_call
								} // end module find
							} // end module.is_open()
						} // end find module name
					} // end find document_root_dir
				} // end find server_name
			} // end foreach applications
		} // end check apllications.size()

		if (apps_tree.empty() )
		{
			std::cout << "Notice: server does not contain applications;" << std::endl;
		}

		return true;
	}

	bool Server::init()
	{
	/*	AllocConsole();
		SetConsoleTitle(L"My Http Server");

		this->hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		this->hInput = GetStdHandle(STD_INPUT_HANDLE); */

		if (0 == Socket::Startup() )
		{
			addDataVariant(new DataVariantFormUrlencoded() );
			addDataVariant(new DataVariantMultipartFormData() );
			addDataVariant(new DataVariantTextPlain() );

			return loadConfig();
		}

		return false;
	}

	void Server::clear()
	{
		Socket::Cleanup();

		if (eventNotFullQueue)
		{
			delete eventNotFullQueue;
			eventNotFullQueue = nullptr;
		}

		if (eventProcessQueue)
		{
			delete eventProcessQueue;
			eventProcessQueue = nullptr;
		}

		if (false == variants.empty() )
		{
			for (auto &variant : variants)
			{
				delete variant.second;
			}

			variants.clear();
		}

		if (false == apps_tree.empty() )
		{
			std::unordered_set<ServerApplicationSettings *> set;
			apps_tree.collectApplicationSettings(set);

			for (auto s : set)
			{
				try
				{
					if (s->application_final)
					{
						s->application_final();
					}
				}
				catch (...)
				{

				}

				delete s;
			}

			set.clear();
			apps_tree.clear();
		}

		if (false == modules.empty() )
		{
			for (auto &module : modules)
			{
				module.close();
			}

			modules.empty();
		}

		if (false == settings.empty() )
		{
			settings.clear();
		}

	//	FreeConsole();
	}

	int Server::help() const
	{
		std::cout << "You call help command" << std::endl;

		return EXIT_SUCCESS;
	}

	int Server::run()
	{
		if (false == init() )
		{
			return 1;
		}

		auto it_option = settings.find("listen");

		if (settings.end() == it_option)
		{
			std::cout << "Error: configuration missing port listen;" << std::endl;
			return 2;
		}

		const int port = std::stoi(it_option->second);

		if (-1 == server_socket.open() )
		{
			std::cout << "Error: cannot open socket; errno " << errno << ";" << std::endl;
			return 3;
		}

		if (-1 == server_socket.bind(port) )
		{
			std::cout << "Error: cannot bind socket " << port << "; errno " << errno << ";" << std::endl;
			return 4;
		}

		if (0 != server_socket.listen() )
		{
			std::cout << "Error: cannot listen socket " << port << "; errno " << errno << ";" << std::endl;
			return 5;
		}

		std::cout << "Log: start server cycle;" << std::endl << std::endl;

		const size_t queue_max_length = 1024;
		eventNotFullQueue = new Event(true, true);
		eventProcessQueue = new Event();

		std::queue<std::shared_ptr<Socket> > sockets;

		process_flag = true;

		std::function<int(Server &, std::queue<std::shared_ptr<Socket> > &)> serverCycleQueue = std::mem_fn(&Server::cycleQueue);
		std::thread threadQueue(serverCycleQueue, std::ref(*this), std::ref(sockets) );

		Socket client_socket;

		while (process_flag)
		{
			eventNotFullQueue->wait();

			client_socket = server_socket.accept();

			if (client_socket.is_open() )
			{
				client_socket.nonblock(true);
				sockets.emplace(new Socket(client_socket) );

				if (sockets.size() <= queue_max_length)
				{
					eventNotFullQueue->reset();
				}

				eventProcessQueue->notify();
			}
		}

		eventProcessQueue->notify();
		threadQueue.join();

		clear();

		std::cout << "Log: final server cycle;" << std::endl;

		return EXIT_SUCCESS;
	}

	void Server::stopProcess()
	{
		process_flag = false;
		server_socket.close();

		if (eventNotFullQueue)
		{
			eventNotFullQueue->notify();
		}
	}

	int Server::start()
	{
		std::string pid_file_name = "httpserver.pid";

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

		process_flag = true;
		restart_flag = false;

		int code = EXIT_FAILURE;

		while (process_flag || restart_flag)
		{
			process_flag = false;
			restart_flag = false;

			code = run();
		}

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
				pid = std::stol(str_pid);
			}
		}

		file.close();

		return pid;
	}

	int Server::restart() const
	{
		const System::native_processid_type pid = getPidFromFile();

		if (1 < pid && System::sendSignal(pid, SIGUSR1) )
		{
			return EXIT_SUCCESS;
		}

		return EXIT_FAILURE;
	}

	int Server::terminate() const
	{
		const System::native_processid_type pid = getPidFromFile();

		if (1 < pid && System::sendSignal(pid, SIGTERM) )
		{
			return EXIT_SUCCESS;
		}

		return EXIT_FAILURE;
	}
};
