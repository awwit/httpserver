
#include "Sendfile.h"
#include "../../../utils/Utils.h"

#include <fstream>

namespace HttpServer
{
	static std::string getMimeTypeByFileName(
		const std::string &fileName,
		const std::unordered_map<std::string, std::string> &mimesTypes
	) noexcept {
		const size_t ext_pos = fileName.rfind('.');

		const std::string file_ext = std::string::npos != ext_pos
			? Utils::getLowerString(fileName.substr(ext_pos + 1) )
			: std::string();

		auto const it_mime = mimesTypes.find(file_ext);

		return mimesTypes.cend() == it_mime
			? std::string("application/octet-stream")
			: it_mime->second;
	}

	static std::vector<std::tuple<size_t, size_t> > getRanges(
		const std::string &rangeHeader,
		const size_t posSymEqual,
		const size_t fileSize,
		std::string *resultRangeHeader,
		size_t *contentLength
	) noexcept {
		std::vector<std::tuple<size_t, size_t> > ranges;

		*contentLength = 0;

		size_t delimiter = posSymEqual; // rangeHeader.find('=');

		const std::string range_unit_name(
			rangeHeader.cbegin(),
			rangeHeader.cbegin() + delimiter
		);

		static const std::unordered_map<std::string, size_t> ranges_units {
			{ "bytes", 1 }
		};

		auto const it_unit = ranges_units.find(range_unit_name);

		if (ranges_units.cend() == it_unit) {
			return ranges;
		}

		const size_t range_unit = it_unit->second;

		for (size_t str_pos; std::string::npos != delimiter; )
		{
			str_pos = delimiter + 1;
			delimiter = rangeHeader.find(',', str_pos);

			const size_t range_pos = rangeHeader.find('-', str_pos);

			if (range_pos < delimiter)
			{
				const std::string range_begin_str(
					rangeHeader.cbegin() + str_pos,
					rangeHeader.cbegin() + range_pos
				);

				const std::string range_end_str(
					rangeHeader.cbegin() + range_pos + 1,
					std::string::npos == delimiter
						? rangeHeader.cend()
						: rangeHeader.cbegin() + delimiter
				);

				if (range_begin_str.empty() == false)
				{
					const size_t range_begin = std::strtoull(
						range_begin_str.c_str(), nullptr, 10
					) * range_unit;

					if (range_begin < fileSize)
					{
						if (range_end_str.empty() == false)
						{
							size_t range_end = std::strtoull(
								range_end_str.c_str(), nullptr, 10
							) * range_unit;

							if (range_end >= range_begin)
							{
								if (range_end > fileSize) {
									range_end = fileSize;
								}

								const size_t length = range_end - range_begin + 1;

								*contentLength += length;

								*resultRangeHeader += std::to_string(range_begin) + '-' + std::to_string(range_end) + ',';

								ranges.emplace_back(std::tuple<size_t, size_t> {
									range_begin, length
								});
							}
						}
						else // if range_end_str empty
						{
							const size_t length = fileSize - range_begin;

							*contentLength += length;

							*resultRangeHeader += std::to_string(range_begin) + '-' + std::to_string(fileSize - 1) + ',';

							ranges.emplace_back(std::tuple<size_t, size_t> {
								range_begin, length
							});
						}
					}
				}
				else if (range_end_str.empty() == false) // if range_begin_str empty
				{
					size_t range_end = std::strtoull(
						range_end_str.c_str(), nullptr, 10
					) * range_unit;

					const size_t length = range_end < fileSize
						? fileSize - range_end
						: fileSize;

					const size_t range_begin = fileSize - length;

					range_end = fileSize - range_begin - 1;

					*contentLength += length;

					*resultRangeHeader += std::to_string(range_begin) + '-' + std::to_string(range_end) + ',';

					ranges.emplace_back(std::tuple<size_t, size_t> {
						range_begin, length
					});
				}
			}
		}

		if (ranges.empty() == false) {
			(*resultRangeHeader).back() = '/';
			*resultRangeHeader = "bytes " + *resultRangeHeader + std::to_string(fileSize);
		}

		return ranges;
	}

	int Sendfile::transferFilePart(
		const ServerProtocol &prot,
		const struct Request &req,
		const std::string &fileName,
		const std::unordered_map<std::string, std::string> &mimesTypes,
		const time_t fileTime,
		const size_t fileSize,
		const std::string &rangeHeader,
		std::vector<std::pair<std::string, std::string> > &additionalHeaders,
		const bool headersOnly
	) noexcept {
		const size_t pos_sym_equal = rangeHeader.find('=');

		if (std::string::npos == pos_sym_equal) {
			prot.sendHeaders(
				Http::StatusCode::BAD_REQUEST,
				additionalHeaders,
				req.timeout
			);

			return 1;
		}

		std::string content_range_header;

		size_t content_length;

		const std::vector<std::tuple<size_t, size_t> > ranges = getRanges(
			rangeHeader,
			pos_sym_equal,
			fileSize,
			&content_range_header,
			&content_length
		);

		if (0 == content_length) {
			prot.sendHeaders(
				Http::StatusCode::REQUESTED_RANGE_NOT_SATISFIABLE,
				additionalHeaders,
				req.timeout
			);

			return 2;
		}

		// Ranges transfer
		std::ifstream file(fileName, std::ifstream::binary);

		if ( ! file) {
			file.close();

			prot.sendHeaders(
				Http::StatusCode::INTERNAL_SERVER_ERROR,
				additionalHeaders,
				req.timeout
			);

			return 3;
		}

		const std::string file_mime_type = getMimeTypeByFileName(fileName, mimesTypes);

		additionalHeaders.emplace_back("content-type", file_mime_type);
		additionalHeaders.emplace_back("content-length", std::to_string(content_length) );
		additionalHeaders.emplace_back("accept-ranges", "bytes");
		additionalHeaders.emplace_back("content-range", content_range_header);
		additionalHeaders.emplace_back("last-modified", Utils::getDatetimeAsString(fileTime, true) );

		// Отправить заголовки (206 Partial Content)
		if (prot.sendHeaders(
				Http::StatusCode::PARTIAL_CONTENT,
				additionalHeaders,
				req.timeout,
				headersOnly
			) == false
		) {
			file.close();
			return 4;
		}

		if (false == headersOnly)
		{
			size_t position, length;

			std::vector<char> buf;

			DataTransfer dt {
				content_length,
				0
			};

			for (auto const &range : ranges)
			{
				std::tie(position, length) = range;

				buf.resize(
					length < 512 * 1024
						? length
						: 512 * 1024
				);

				file.seekg(position, file.beg);

				size_t send_size_left = length;

				long send_size;

				do {
					if (send_size_left < 512 * 1024) {
						buf.resize(send_size_left);
					}

					file.read(buf.data(), buf.size() );

					auto const read_size = file.gcount();

					send_size = prot.sendData(
						buf.data(),
						read_size,
						req.timeout,
						&dt
					);

					send_size_left -= send_size;
				}
				while (
					false == file.eof() &&
					false == file.fail() &&
					send_size > 0 &&
					send_size_left
				);
			}
		}

		file.close();

		return 0;
	}

	/**
	 * Передача файла (или его части)
	 */
	int Sendfile::transferFile(
		const ServerProtocol &prot,
		const struct Request &req,
		const std::string &fileName,
		const std::unordered_map<std::string, std::string> &mimesTypes,
		std::vector<std::pair<std::string, std::string> > &additionalHeaders,
		const bool headersOnly
	) noexcept {
		// Get current time in GMT
		additionalHeaders.emplace_back("date", Utils::getDatetimeAsString() );

		time_t file_time;
		size_t file_size;

		// Получить размер файла и дату последнего изменения
		if (System::getFileSizeAndTimeGmt(fileName, &file_size, &file_time) == false) {
			prot.sendHeaders(
				Http::StatusCode::NOT_FOUND,
				additionalHeaders,
				req.timeout
			);

			return 1;
		}

		// Check for If-Modified header
		auto const it_modified = req.incoming_headers.find("if-modified-since");

		// Если найден заголовок проверки изменения файла (проверить, изменялся ли файл)
		if (req.incoming_headers.cend() != it_modified) {
			const time_t time_in_request = Utils::rfc822DatetimeToTimestamp(it_modified->second);

			if (file_time == time_in_request) {
				prot.sendHeaders(
					Http::StatusCode::NOT_MODIFIED,
					additionalHeaders,
					req.timeout
				);

				return 2;
			}
		}

		auto const it_range = req.incoming_headers.find("range");

		// Range transfer
		if (req.incoming_headers.cend() != it_range) {
			return transferFilePart(
				prot,
				req,
				fileName,
				mimesTypes,
				file_time,
				file_size,
				it_range->second,
				additionalHeaders,
				headersOnly
			);
		}

		// File transfer
		std::ifstream file(fileName, std::ifstream::binary);

		if ( ! file) {
			file.close();

			prot.sendHeaders(
				Http::StatusCode::INTERNAL_SERVER_ERROR,
				additionalHeaders,
				req.timeout
			);

			return 3;
		}

		const std::string file_mime_type = getMimeTypeByFileName(fileName, mimesTypes);

		additionalHeaders.emplace_back("content-type", file_mime_type);
		additionalHeaders.emplace_back("content-length", std::to_string(file_size) );
		additionalHeaders.emplace_back("accept-ranges", "bytes");
		additionalHeaders.emplace_back("last-modified", Utils::getDatetimeAsString(file_time, true) );

		// Отправить заголовки (200 OK)
		if (prot.sendHeaders(
				Http::StatusCode::OK,
				additionalHeaders,
				req.timeout,
				headersOnly || 0 == file_size
			) == false
		) {
			file.close();
			return 4;
		}

		// Отправить файл
		if (false == headersOnly && file_size)
		{
			std::vector<char> buf(
				file_size < 512 * 1024
					? file_size
					: 512 * 1024
			);

			long send_size;

			DataTransfer dt {
				file_size,
				0
			};

			do {
				file.read(buf.data(), buf.size() );

				send_size = prot.sendData(
					buf.data(),
					file.gcount(),
					req.timeout,
					&dt
				);
			}
			while (
				false == file.eof() &&
				false == file.fail() &&
				send_size > 0 &&
				(dt.full_size - dt.send_total)
			);
		}

		file.close();

		return 0;
	}

	bool Sendfile::isConnectionReuse(const struct Request &req) noexcept {
		return (req.connection_params & ConnectionParams::CONNECTION_REUSE) == ConnectionParams::CONNECTION_REUSE;
	}

	void Sendfile::xSendfile(
		const ServerProtocol &prot,
		struct Request &req,
		const std::unordered_map<std::string, std::string> &mimesTypes
	) noexcept {
		auto const it_x_sendfile = req.outgoing_headers.find("x-sendfile");

		if (req.outgoing_headers.cend() != it_x_sendfile)
		{
			std::vector<std::pair<std::string, std::string> > additional_headers;

			if (Transfer::ProtocolVariant::HTTP_1 == req.protocol_variant) {
				if (isConnectionReuse(req) ) {
					additional_headers.emplace_back("connection", "keep-alive");
					additional_headers.emplace_back("keep-alive", "timeout=5; max=" + std::to_string(req.keep_alive_count) );
				} else {
					additional_headers.emplace_back("connection", "close");
				}
			}

			const bool headers_only = ("head" == req.method);

			transferFile(
				prot,
				req,
				it_x_sendfile->second,
				mimesTypes,
				additional_headers,
				headers_only
			);
		}
	}
}
