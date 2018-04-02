
#include "ServerHttp2Protocol.h"
#include "../../utils/Utils.h"
#include "../../transfer/http2/HPack.h"

#include <random>

namespace HttpServer
{
	ServerHttp2Protocol::ServerHttp2Protocol(
		Socket::Adapter &sock,
		const ServerSettings &settings,
		ServerControls &controls,
		Http2::IncStream *stream
	) noexcept
		: ServerProtocol(sock, settings, controls), stream(stream)
	{

	}

	uint8_t ServerHttp2Protocol::getPaddingSize(const size_t dataSize)
	{
		if (0 == dataSize) {
			return 0;
		}

		std::random_device rd;

		uint8_t padding = uint8_t(rd());

		while (dataSize <= padding) {
			padding /= 2;
		}

		return padding;
	}

	bool ServerHttp2Protocol::sendHeaders(
		const Http::StatusCode status,
		std::vector<std::pair<std::string, std::string> > &headers,
		const std::chrono::milliseconds &timeout,
		const bool endStream
	) const {
		headers.emplace(
			headers.begin(),
			":status",
			std::to_string(static_cast<int>(status))
		);

		std::vector<char> buf;
		buf.reserve(4096);
		buf.resize(Http2::FRAME_HEADER_SIZE);

		HPack::pack(buf, headers, this->stream->conn.encoding_dynamic_table);

		const uint32_t frame_size = uint32_t(
			buf.size() - Http2::FRAME_HEADER_SIZE
		);

		Http2::FrameFlag flags = Http2::FrameFlag::END_HEADERS;

		if (endStream) {
			flags |= Http2::FrameFlag::END_STREAM;
		}

		this->stream->setHttp2FrameHeader(
			reinterpret_cast<uint8_t *>(buf.data() ),
			frame_size,
			Http2::FrameType::HEADERS,
			flags
		);

		const std::unique_lock<std::mutex> lock(this->stream->conn.sync.mtx);

		return this->sock.nonblock_send(buf.data(), buf.size(), timeout) > 0;
	}

	long ServerHttp2Protocol::sendData(
		const void *src,
		size_t size,
		const std::chrono::milliseconds &timeout,
		DataTransfer *dt
	) const {
		const uint8_t *data = reinterpret_cast<const uint8_t *>(src);

		const Http2::ConnectionSettings &setting = this->stream->conn.client_settings;

		std::vector<uint8_t> buf;
		buf.reserve(setting.max_frame_size + Http2::FRAME_HEADER_SIZE);

		long send_size = 0;

		while (size != 0)
		{
			// TODO: test with data_size == 1 (padding length == 0)
			size_t data_size = setting.max_frame_size < size
				? setting.max_frame_size
				: size;

			const uint8_t padding = getPaddingSize(data_size);
			const uint16_t padding_size = padding + sizeof(uint8_t);

			if (padding_size) {
				if (data_size + padding_size > setting.max_frame_size) {
					data_size = setting.max_frame_size - padding_size;
				}
			}

			const uint32_t frame_size = static_cast<uint32_t>(
				data_size + padding_size
			);

			buf.resize(frame_size + Http2::FRAME_HEADER_SIZE);

		/*	if (this->stream->window_size_out - frame_size <= 0)
			{
				size_t update_size = (dt->full_size - dt->send_total) - this->stream->window_size_out;

				if (update_size > Http2::MAX_WINDOW_UPDATE) {
					update_size = Http2::MAX_WINDOW_UPDATE;
				}

				sendWindowUpdate(this->sock, rp, uint32_t(update_size) );

				this->stream->window_size_out += update_size;
			}*/

			Http2::FrameFlag flags = Http2::FrameFlag::EMPTY;

			if (dt->send_total + data_size >= dt->full_size) {
				flags |= Http2::FrameFlag::END_STREAM;
			}

			size_t cur = Http2::FRAME_HEADER_SIZE;

			if (padding_size) {
				flags |= Http2::FrameFlag::PADDED;

				buf[cur] = padding;

				++cur;
			}

			this->stream->setHttp2FrameHeader(
				buf.data(),
				frame_size,
				Http2::FrameType::DATA,
				flags
			);

			std::copy(
				data,
				data + data_size,
				buf.data() + cur
			);

			if (padding) {
				std::fill(
					buf.end() - padding,
					buf.end(),
					0
				);
			}

			this->stream->lock();

			const long sended = this->sock.nonblock_send(
				buf.data(),
				buf.size(),
				timeout
			);

			this->stream->unlock();

			if (sended <= 0) {
				send_size = sended;
				break;
			}

			data += data_size;
			send_size += long(data_size);
			dt->send_total += data_size;
		//	stream->window_size_out -= frame_size;

			size -= data_size;
		}

		return send_size;
	}

	bool ServerHttp2Protocol::packRequestParameters(
		std::vector<char> &buf,
		const struct Request &req,
		const std::string &rootDir
	) const {
		Utils::packNumber(buf, static_cast<size_t>(Transfer::ProtocolVariant::HTTP_2) );
		Utils::packString(buf, rootDir);
		Utils::packString(buf, req.host);
		Utils::packString(buf, req.path);
		Utils::packString(buf, req.method);

		Utils::packNumber(buf, this->stream->stream_id);
		Utils::packNumber(buf, this->stream->conn.client_settings.header_table_size);
		Utils::packNumber(buf, this->stream->conn.client_settings.enable_push);
		Utils::packNumber(buf, this->stream->conn.client_settings.max_concurrent_streams);
		Utils::packNumber(buf, this->stream->conn.client_settings.initial_window_size);
		Utils::packNumber(buf, this->stream->conn.client_settings.max_frame_size);
		Utils::packNumber(buf, this->stream->conn.client_settings.max_header_list_size);
		Utils::packContainer(buf, this->stream->conn.encoding_dynamic_table.getList() );
		Utils::packPointer(buf, &this->stream->conn.sync.mtx);
		Utils::packContainer(buf, req.incoming_headers);
		Utils::packContainer(buf, req.incoming_data);
		Utils::packFilesIncoming(buf, req.incoming_files);

		return true;
	}

	void ServerHttp2Protocol::unpackResponseParameters(
		struct Request &req,
		const void *src
	) const {
		Utils::unpackContainer(
			req.outgoing_headers,
			reinterpret_cast<const uint8_t *>(src)
		);
	}
}
