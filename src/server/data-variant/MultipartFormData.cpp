
#include "../../utils/Utils.h"
#include "MultipartFormData.h"

#include <fstream>

namespace DataVariant
{
	MultipartFormData::MultipartFormData() noexcept {
		this->data_variant_name = "multipart/form-data";
	}

	enum class ParsingState : uint8_t {
		INITIALIZATION = 0,
		FIND_DATA_BLOCK,
		GET_DATA_BLOCK_TYPE,
		COPY_DATA_BLOCK,
		SAVE_DATA_BLOCK,
	};

	enum class BlockType : uint8_t {
		UNKNOWN = 0,
		DATA,
		FILE,
	};

	struct StateMultipartFormData {
		std::string block_name;
		std::string block_value;
		std::string file_tmp_name;
		std::string file_name;
		std::string file_type;
		std::ofstream file;
		std::string boundary;
		ParsingState state;
		BlockType block_type;
	};

	void *MultipartFormData::createStateStruct(
		const Transfer::request_data *rd,
		const std::unordered_map<std::string, std::string> &contentParams
	) const {
		std::string boundary;

		auto const it = contentParams.find("boundary");

		if (contentParams.cend() != it) {
			boundary = it->second;
		}

		return new StateMultipartFormData {
			std::string(), std::string(), std::string(), std::string(), std::string(), std::ofstream(),
			std::move(boundary), ParsingState::INITIALIZATION, BlockType::UNKNOWN
		};
	}

	void MultipartFormData::destroyStateStruct(void *st) const noexcept {
		delete reinterpret_cast<StateMultipartFormData *>(st);
	}

	static std::unordered_map<std::string, std::string> parseHeader(const std::string &buf, size_t cur, const size_t end)
	{
		const std::string nl("\r\n");

		std::unordered_map<std::string, std::string> headers;

		for (
			 size_t line_end = buf.find(nl.data(), cur);
			 cur < end;
			 line_end = buf.find(nl.data(), cur)
		) {
			size_t delimiter = buf.find(':', cur);

			if (std::string::npos == delimiter || delimiter > line_end)
			{
				std::string header_name = buf.substr(cur, line_end - cur);
				Utils::trim(header_name);
				Utils::toLower(header_name);

				headers.emplace(
					std::move(header_name),
					std::string()
				);
			} else {
				std::string header_name = buf.substr(cur, delimiter - cur);
				Utils::trim(header_name);
				Utils::toLower(header_name);

				++delimiter;

				std::string header_value = buf.substr(delimiter, line_end - delimiter);
				Utils::trim(header_value);

				headers.emplace(
					std::move(header_name),
					std::move(header_value)
				);
			}

			// Перейти к следующему заголовку
			cur = line_end + nl.length();
		}

		return headers;
	}

	static std::unordered_map<std::string, std::string> parseMainHeaderParams(const std::string &header)
	{
		// Разобрать значение заголовка данных на параметры
		std::unordered_map<std::string, std::string> header_params;

		size_t delimiter = header.find(';');

		if (std::string::npos == delimiter) {
			return header_params;
		}

		std::string content_disposition(header.substr(0, delimiter) );
		Utils::trim(content_disposition);

		// Проверить соответствие указанного формата
		if ("form-data" != content_disposition) {
			return header_params;
		}

		// Получить параметры блока данных
		for (size_t cur = delimiter + 1, end; std::string::npos != cur; cur = end)
		{
			end = header.find(';', cur);
			delimiter = header.find('=', cur);

			if (std::string::npos == delimiter || delimiter > end)
			{
				std::string param_name = header.substr(
					cur,
					std::string::npos != end
						? end - cur
						: std::string::npos
				);

				Utils::trim(param_name);
				Utils::toLower(param_name);

				header_params.emplace(
					std::move(param_name),
					std::string()
				);
			} else {
				std::string param_name = header.substr(cur, delimiter - cur);
				Utils::trim(param_name);
				Utils::toLower(param_name);

				++delimiter;

				delimiter = header.find('"', delimiter);

				if (std::string::npos == delimiter)
				{
					end = header.find(';', cur);

					std::string param_value = header.substr(
						delimiter,
						std::string::npos != end
							? end - delimiter
							: std::string::npos
					);

					Utils::trim(param_value);

					header_params.emplace(
						std::move(param_name),
						std::move(param_value)
					);
				} else {
					++delimiter;

					cur = header.find('"', delimiter);
					end = header.find(';', cur);

					std::string param_value = header.substr(
						delimiter,
						std::string::npos != cur
							? cur - delimiter
							: std::string::npos
					);

					header_params.emplace(
						std::move(param_name),
						std::move(param_value)
					);
				}
			}

			if (std::string::npos != end) {
				++end;
			}
		}

		return header_params;
	}

	bool MultipartFormData::parse(
		const std::string &buf,
		Transfer::request_data *rd,
		DataReceiver *dr
	) const {
		StateMultipartFormData *ss = reinterpret_cast<StateMultipartFormData *>(dr->ss);

		size_t cur = 0;

		while (cur < buf.size() )
		{
			switch (ss->state)
			{
				case ParsingState::INITIALIZATION:
				{
					if (ss->boundary.empty() ) {
						return false;
					}

					const std::string data_end("--" + ss->boundary + "--\r\n");

					if (buf.size() < data_end.length() ) {
						dr->left = buf.size();
						return dr->full_size != dr->recv_total;
					}

					if (0 == buf.find(data_end) ) {
						return dr->full_size == data_end.length() && dr->full_size == dr->recv_total;
					}

					const std::string first_block("--" + ss->boundary);

					cur = buf.find(first_block);

					if (0 != cur) {
						return false;
					}

					cur += first_block.length() + 2;

					ss->state = ParsingState::GET_DATA_BLOCK_TYPE;

					break;
				}

				case ParsingState::FIND_DATA_BLOCK:
				{
					dr->left = buf.size() - cur;

					const std::string data_end("\r\n--" + ss->boundary + "--\r\n");

					if (data_end.length() > dr->left) {
						return dr->full_size != dr->recv_total;
					}

					const size_t end = buf.find(data_end, cur);

					if (end == cur) {
						dr->left -= data_end.length();
						return dr->full_size == dr->recv_total;
					}

					const std::string block_delimiter("\r\n--" + ss->boundary);

					cur = buf.find(block_delimiter, cur);

					if (std::string::npos == cur) {
						return dr->full_size != dr->recv_total;
					}

					cur += block_delimiter.length() + 2;

					ss->state = ParsingState::GET_DATA_BLOCK_TYPE;

				//	break;
				}

				case ParsingState::GET_DATA_BLOCK_TYPE:
				{
					const size_t end = buf.find("\r\n\r\n", cur);

					if (std::string::npos == end) {
						dr->left = buf.size() - cur;
						return dr->full_size != dr->recv_total;
					}

					// Разобрать заголовки блока данных
					const std::unordered_map<std::string, std::string> headers = parseHeader(buf, cur, end);

					// Определить параметры блока данных
					auto const it = headers.find("content-disposition");

					// Если заголовок не определён
					if (headers.cend() == it) {
						return false;
					}

					const std::unordered_map<std::string, std::string> header_params = parseMainHeaderParams(it->second);

					// Поиск имени блока данных
					auto const it_name = header_params.find("name");

					if (header_params.cend() == it_name) {
						return false;
					}

					ss->block_name = it_name->second;

					auto const it_filename = header_params.find("filename");

					ss->block_type = header_params.cend() == it_filename ? BlockType::DATA : BlockType::FILE;

					if (BlockType::FILE == ss->block_type)
					{
						ss->file_name = it_filename->second;

						// Найти тип файла
						auto const it_filetype = headers.find("content-type");

						if (headers.cend() != it_filetype) {
							ss->file_type = it_filetype->second;
						}

						// Сгенерировать уникальное имя
						ss->file_tmp_name = System::getTempDir() + Utils::getUniqueName();

						// Создать файл
						ss->file.open(ss->file_tmp_name, std::ofstream::trunc | std::ofstream::binary);

						if (ss->file.is_open() == false) {
							return false;
						}
					}

					// Перейти к данным
					cur = end + 4;

					ss->state = ParsingState::COPY_DATA_BLOCK;

				//	break;
				}

				case ParsingState::COPY_DATA_BLOCK:
				{
					const std::string block_delimiter("\r\n--" + ss->boundary);

					// Поиск конца блока данных (возможное начало следующего блока)
					const size_t block_end = buf.find(block_delimiter, cur);

					const size_t end = (std::string::npos == block_end) ? buf.size() - block_delimiter.length() : block_end;

					switch (ss->block_type)
					{
						case BlockType::DATA: {
							ss->block_value.append(
								buf.cbegin() + long(cur),
								buf.cbegin() + long(end)
							);

							break;
						}

						case BlockType::FILE: {
							ss->file.write(
								&buf[cur],
								std::streamsize(end - cur)
							);

							break;
						}

						default:
							return false;
					}

					if (std::string::npos == block_end) {
						dr->left = buf.size() - end;
						return dr->full_size != dr->recv_total;
					}

					cur = end;

					ss->state = ParsingState::SAVE_DATA_BLOCK;

				//	break;
				}

				case ParsingState::SAVE_DATA_BLOCK:
				{
					switch (ss->block_type)
					{
						case BlockType::DATA: {
							rd->incoming_data.emplace(
								std::move(ss->block_name),
								std::move(ss->block_value)
							);

							break;
						}

						case BlockType::FILE: {
							rd->incoming_files.emplace(
								std::move(ss->block_name),
								Transfer::FileIncoming(
									std::move(ss->file_tmp_name),
									std::move(ss->file_name),
									std::move(ss->file_type),
									size_t(ss->file.tellp())
								)
							);

							ss->file.close();
							break;
						}

						default:
							return false;
					}

					ss->state = ParsingState::FIND_DATA_BLOCK;

					break;
				}

				default:
					return false;
			}
		}

		return dr->full_size != dr->recv_total;
	}
}
