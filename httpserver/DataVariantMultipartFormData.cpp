
#include "Utils.h"
#include "DataVariantMultipartFormData.h"

#include <fstream>

namespace HttpServer
{
	DataVariantMultipartFormData::DataVariantMultipartFormData()
	{
		data_variant_name = "multipart/form-data";
	}

	bool DataVariantMultipartFormData::append
	(
		const Socket &sock,
		const std::chrono::milliseconds &timeout,
		std::vector<std::string::value_type> &buf,
		std::string &str_buf,
		const std::string &data_end,
		const size_t &leftBytes,
		size_t &recv_len,
		size_t &read_len
	) const
	{
		// Завершаем работу, если уже получено байт сколько нужно
		if (read_len >= leftBytes)
		{
			return false;
		}

		// Получаем данные из сокета
		recv_len = sock.nonblock_recv(buf, timeout);

		// Завершаем работу, если ошибка получения данных через сокет
		if (0 == recv_len || std::numeric_limits<size_t>::max() == recv_len)
		{
			return false;
		}

		// Обновляем общее количество полученных данных
		read_len += recv_len;

		// Добавляем полученные данные к рабочему буферу
		str_buf.append(buf.cbegin(), buf.cbegin() + recv_len);

		// Завершаем работу, если в буфере меньше чем окончание блока данных
		if (str_buf.length() <= data_end.length() )
		{
			return false;
		}

		return true;
	}

	bool DataVariantMultipartFormData::parse
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
		// Проверить есть ли в параметрах разделитель блоков данных
		auto it = params.find("boundary");

        if (params.cend() == it)
		{
			return false;
		}

		// Определить признак начала блока данных
		std::string block_delimiter("--" + it->second);
		// Определить признак окончания всех данных
		std::string data_end("--" + it->second + "--\r\n");

		if (0 == str.find(data_end) )
		{
			return true;
		}

		data_end = "\r\n" + data_end;

		// Установить размер буфера данных
		const size_t buf_len = (leftBytes >= 512 * 1024) ? 512 * 1024 : leftBytes;

		// Создание буферов
		std::vector<std::string::value_type> buf(buf_len);

		size_t str_cur;			// Текущая позиция в буфере

		size_t recv_len;		// Прочитано байт при последнем извлечении данных из сокета
		size_t read_len = 0;	// Прочитано байт из сокета всего

		// Поиск разделителя блока данных
		str_cur = str.find(block_delimiter);

		if (std::string::npos == str_cur)
		{
			// Получить следующий кусок данных
			if (false == append(sock, timeout, buf, str, data_end, leftBytes, recv_len, read_len) )
			{
				return false;
			}

			// Поиск разделителя блока данных
			str_cur = str.find(block_delimiter);

			if (std::string::npos == str_cur)
			{
				return false;
			}
		}

		str_cur += block_delimiter.length() + 2;

		block_delimiter = "\r\n" + block_delimiter;

		// Если найден конец данных
		bool is_find_data_end = false;

		do
		{
			// Правильно ли был передан блок данных
			bool is_block_valid = true;

			// Поиск конца заголовков блока данных
			size_t headers_end = str.find("\r\n\r\n", str_cur);

			// Если конец не был найден, то
			if (std::string::npos == headers_end)
			{
				// Получить следующий кусок данных
				if (false == append(sock, timeout, buf, str, data_end, leftBytes, recv_len, read_len) )
				{
					return false;
				}

				// Провести повторный поиск
				headers_end = str.find("\r\n\r\n", str_cur);

				// Если снова не найдено, то данные некорректны
				if (std::string::npos == headers_end)
				{
					is_block_valid = false;
				}
			}

			if (is_block_valid)
			{
				// Разобрать заголовки блока данных
				std::unordered_map<std::string, std::string> headers;

				for (size_t line_end = str.find("\r\n", str_cur); str_cur < headers_end; line_end = str.find("\r\n", str_cur) )
				{
					size_t delimiter = str.find(':', str_cur);

					if (std::string::npos == delimiter || delimiter > line_end)
					{
						std::string header_name = str.substr(str_cur, line_end - str_cur);
						Utils::trim(header_name);
						headers.emplace(std::move(header_name), "");
					}
					else
					{
						std::string header_name = str.substr(str_cur, delimiter - str_cur);
						Utils::trim(header_name);

						++delimiter;

						std::string header_value = str.substr(delimiter, line_end - delimiter);
						Utils::trim(header_value);

						headers.emplace(std::move(header_name), std::move(header_value) );
					}

					// Перейти к следующему заголовку
					str_cur = line_end + 2;
				}

				// Перейти к данным
				str_cur += 2;

				// Определить источник блока данных
				auto it = headers.find("Content-Disposition");

				// Если заголовок определён
                if (headers.cend() != it)
				{
					// Разобрать значение заголовка данных на параметры
					std::unordered_map<std::string, std::string> header_params;

					std::string &header_value = it->second;

					size_t delimiter = header_value.find(';');

					std::string content_disposition(header_value.substr(0, delimiter) );
					Utils::trim(content_disposition);

					// Проверить соответствие указанного формата
					if ("form-data" == content_disposition && std::string::npos != delimiter)
					{
						// Получить параметры блока данных
						for (size_t str_param_cur = delimiter + 1, str_param_end; std::string::npos != str_param_cur; str_param_cur = str_param_end)
						{
							str_param_end = header_value.find(';', str_param_cur);
							delimiter = header_value.find('=', str_param_cur);

							if (std::string::npos == delimiter || delimiter > str_param_end)
							{
								std::string param_name = (std::string::npos == str_param_end) ? header_value.substr(str_param_cur) : header_value.substr(str_param_cur, str_param_end - str_param_cur);
								Utils::trim(param_name);
								header_params.emplace(std::move(param_name), "");
							}
							else
							{
								std::string param_name = header_value.substr(str_param_cur, delimiter - str_param_cur);
								Utils::trim(param_name);

								++delimiter;

								delimiter = header_value.find('"', delimiter);

								if (std::string::npos == delimiter)
								{
									str_param_end = header_value.find(';', str_param_cur);

									std::string param_value = (std::string::npos == str_param_end) ? header_value.substr(delimiter) : header_value.substr(delimiter, str_param_end - delimiter);
									Utils::trim(param_value);

									header_params.emplace(std::move(param_name), std::move(param_value) );
								}
								else
								{
									++delimiter;

									str_param_cur = header_value.find('"', delimiter);
									str_param_end = header_value.find(';', str_param_cur);

									std::string param_value = (std::string::npos == str_param_cur) ? header_value.substr(delimiter) : header_value.substr(delimiter, str_param_cur - delimiter);

									header_params.emplace(std::move(param_name), std::move(param_value) );
								}
							}

							if (std::string::npos != str_param_end)
							{
								++str_param_end;
							}
						}

						// Поиск имени блока данных
						auto it_name = header_params.find("name");

                        if (header_params.cend() != it_name)
						{
							// Если данные пришли из файла
							auto it_filename = header_params.find("filename");

                            if (header_params.cend() != it_filename)
							{
								// Найти тип файла
								auto it_filetype = headers.find("Content-Type");

                                if (headers.cend() != it_filetype)
								{
									// Сгенерировать уникальное имя
									std::string tmp_name = System::getTempDir() + Utils::getUniqueName();

									// Создать файл
									std::ofstream file(tmp_name, std::ofstream::trunc | std::ofstream::binary);

									// Если файл был создан и готов для работы
									if (file.is_open() )
									{
										// Смещение данных в буфере в начало
//										str.assign(str.cbegin() + str_cur, str.cend() );
										str.erase(str.begin(), str.begin() + str_cur);

										// Поиск конца блока данных
										size_t delimiter = str.find(block_delimiter);

										// Пока конец блока данных не найден
										while (std::string::npos == delimiter)
										{
											// Добавить данные к значению
											file.write(str.data(), str.length() - data_end.length() );

//											str.assign(str.cend() - data_end.length(), str.cend() );
											str.erase(str.begin(), str.end() - data_end.length() );

											// Получить следующий кусок данных
											if (false == append(sock, timeout, buf, str, data_end, leftBytes, recv_len, read_len) )
											{
												return false;
											}

											// Поиск конца блока данных
											delimiter = str.find(block_delimiter);
										}

										// Добавить последнюю часть данных к значению
										file.write(str.data(), delimiter);

										// Добавить данные в список
										files.emplace(it_name->second, FileIncoming(std::move(tmp_name), it_filetype->second, file.tellp() ) );

										file.close();

										// Если найден конец данных
										if (str.find(data_end, delimiter) == delimiter)
										{
											is_find_data_end = true;
										}

										str_cur = delimiter + block_delimiter.length() + 2;
									}
									else // Файл не смог быть открыт/создан
									{
										is_block_valid = false;
									}
								}
								else // Тип файла не определён
								{
									is_block_valid = false;
								}
							}
							else // Если данные пришли из формы
							{
								std::string value;

								// Смещение данных в буфере в начало
//								str.assign(str.cbegin() + str_cur, str.cend() );
								str.erase(str.begin(), str.begin() + str_cur);

								// Поиск конца блока данных
								size_t delimiter = str.find(block_delimiter);

								// Пока конец блока данных не найден
								while (std::string::npos == delimiter)
								{
									// Добавить данные к значению
									value.append(str.cbegin(), str.cend() - data_end.length() );

//									str.assign(str.cend() - data_end.length(), str.cend() );
									str.erase(str.begin(), str.end() - data_end.length() );

									// Получить следующий кусок данных
									if (false == append(sock, timeout, buf, str, data_end, leftBytes, recv_len, read_len) )
									{
										return false;
									}

									// Поиск конца блока данных
									delimiter = str.find(block_delimiter);
								}

								// Добавить последнюю часть данных к значению
								value.append(str.cbegin(), str.cbegin() + delimiter);

								// Добавить данные в список
								data.emplace(it_name->second, std::move(value) );

								// Если найден конец данных
								if (str.find(data_end, delimiter) == delimiter)
								{
									is_find_data_end = true;
								}

								str_cur = delimiter + block_delimiter.length() + 2;
							}
						}
						else // Имя блока данных не определено
						{
							is_block_valid = false;
						}
					}
					else // Формат не соответствует
					{
						is_block_valid = false;
					}
				}
				else // Если источник не определён
				{
					is_block_valid = false;
				}
			}

			// Если данные некорректны
			if (false == is_block_valid)
			{
				// то блок данных пропускаем (ищем следующий блок)
				str_cur = str.find(block_delimiter, str_cur);

				while (std::string::npos == str_cur)
				{
//					str.assign(str.cend() - data_end.length(), str.cend() );
					str.erase(str.begin(), str.end() - data_end.length() );

					// Получить следующий кусок данных
					if (false == append(sock, timeout, buf, str, data_end, leftBytes, recv_len, read_len) )
					{
						return false;
					}

					str_cur = str.find(block_delimiter);
				}

				// Если найден конец данных
				if (str.find(data_end, str_cur) == str_cur)
				{
					is_find_data_end = true;
				}

				str_cur += block_delimiter.length() + 2;
			}

			str.erase(str.begin(), str.begin() + str_cur);

			str_cur = 0;
		}
		while (false == is_find_data_end);

		return true;
	}
};