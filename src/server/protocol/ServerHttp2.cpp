
#include "ServerHttp2.h"
#include "../../utils/Utils.h"
#include "../../transfer/http2/HPack.h"

#include <array>

namespace HttpServer
{
	ServerHttp2::ServerHttp2(
		Socket::Adapter &sock,
		const ServerSettings &settings,
		ServerControls &controls,
		SocketsQueue &sockets
	) noexcept
		: ServerHttp2Protocol(sock, settings, controls, nullptr), sockets(sockets)
	{

	}

	void ServerHttp2::close() {
		this->sock.close();
	}

	static uint8_t *setHttp2FrameHeader(
		uint8_t *addr,
		const uint32_t frameSize,
		const Http2::FrameType frameType,
		const Http2::FrameFlag frameFlags,
		const uint32_t streamId
	) noexcept {
		Utils::hton24(addr, frameSize);
		*(addr + 3) = static_cast<const uint8_t>(frameType);
		*(addr + 4) = static_cast<const uint8_t>(frameFlags);
		*reinterpret_cast<uint32_t *>(addr + 5) = ::htonl(streamId);

		return (addr + Http2::FRAME_HEADER_SIZE);
	}

	static Http2::IncStream &getStreamData(
		std::unordered_map<uint32_t, Http2::IncStream> &streams,
		const uint32_t streamId,
		Http2::ConnectionData &conn
	) noexcept {
		auto it = streams.find(streamId);

		if (streams.end() != it) {
			return it->second;
		}

		return streams.emplace(streamId, Http2::IncStream(streamId, conn) ).first->second;
	}

	static void sendWindowUpdate(
		const Socket::Adapter &sock,
		const std::chrono::milliseconds &timeout,
		const Http2::IncStream &stream,
		const uint32_t size
	) noexcept {
		std::array<uint8_t, Http2::FRAME_HEADER_SIZE + sizeof(uint32_t)> buf;
		uint8_t *addr = buf.data();

		addr = setHttp2FrameHeader(
			addr,
			sizeof(uint32_t),
			Http2::FrameType::WINDOW_UPDATE,
			Http2::FrameFlag::EMPTY,
			stream.stream_id
		);

		*reinterpret_cast<uint32_t *>(addr) = ::htonl(size);

		const std::unique_lock<std::mutex> lock(stream.conn.sync.mtx);

		sock.nonblock_send(buf.data(), buf.size(), timeout);
	}

	static Http2::ErrorCode parseHttp2Data(
		Http2::FrameMeta &meta,
		Http2::IncStream &stream,
		const uint8_t *src,
		const uint8_t *end
	) {
		if (0 == meta.stream_id) {
			return Http2::ErrorCode::PROTOCOL_ERROR;
		}

		if (Http2::StreamState::OPEN != stream.state) {
			return Http2::ErrorCode::STREAM_CLOSED;
		}

		if (stream.window_size_inc <= 0) {
			return Http2::ErrorCode::FLOW_CONTROL_ERROR;
		}

		uint8_t padding = 0;

		if (meta.flags & Http2::FrameFlag::PADDED)
		{
			padding = *src;

			if (padding >= meta.length) {
				return Http2::ErrorCode::PROTOCOL_ERROR;
			}

			src += sizeof(uint8_t);
		}

		Http2::ErrorCode error_code = Http2::ErrorCode::NO_ERROR;

		if (stream.reserved)
		{
			Transfer::request_data *rd = static_cast<Transfer::request_data *>(&stream);

			DataVariant::DataReceiver *dr = reinterpret_cast<DataVariant::DataReceiver *>(stream.reserved);

			std::string &buf = *reinterpret_cast<std::string *>(dr->reserved);

			buf.append(src, end - padding);

			dr->recv_total += size_t(end - src) - padding;

			if (dr->data_variant->parse(buf, rd, dr) ) {
				buf.erase(
					0, buf.length() - dr->left
				);
			} else {
				error_code = Http2::ErrorCode::PROTOCOL_ERROR;
			}
		} else {
			error_code = Http2::ErrorCode::PROTOCOL_ERROR;
		}

		if (meta.flags & Http2::FrameFlag::END_STREAM) {
			stream.state = Http2::StreamState::HALF_CLOSED;

			ServerProtocol::destroyDataReceiver(stream.reserved);
			stream.reserved = nullptr;
		}

		return error_code;
	}

	static Http2::ErrorCode parseHttp2Headers(
		Http2::FrameMeta &meta,
		Http2::IncStream &stream,
		const uint8_t *src,
		const uint8_t *end
	) {
		stream.state = (meta.flags & Http2::FrameFlag::END_STREAM)
			? Http2::StreamState::HALF_CLOSED
			: Http2::StreamState::OPEN;

		uint8_t padding = 0;

		if (meta.flags & Http2::FrameFlag::PADDED)
		{
			padding = *src;

			if (padding >= meta.length) {
				return Http2::ErrorCode::PROTOCOL_ERROR;
			}

			src += sizeof(uint8_t);
		}

		if (meta.flags & Http2::FrameFlag::PRIORITY)
		{
			// Stream id
			const uint32_t depend_stream_id = ::ntohl(
				*reinterpret_cast<const uint32_t *>(src)
			) & ~(uint32_t(1) << 31);

			src += sizeof(uint32_t);

			// Priority weight
			stream.priority = *src;

			src += sizeof(uint8_t);
		}

		if (HPack::unpack(src, size_t(end - src) - padding, stream) == false) {
			return Http2::ErrorCode::COMPRESSION_ERROR;
		}

		return Http2::ErrorCode::NO_ERROR;
	}

	static Http2::ErrorCode parseHttp2rstStream(
		Http2::FrameMeta &meta,
		Http2::IncStream &stream,
		const uint8_t *src,
		const uint8_t *end
	) {
		if (Http2::StreamState::IDLE == stream.state) {
			return Http2::ErrorCode::PROTOCOL_ERROR;
		}

		stream.state = Http2::StreamState::CLOSED;

		if (0 == meta.stream_id) {
			return Http2::ErrorCode::PROTOCOL_ERROR;
		}

		if (sizeof(uint32_t) != meta.length) {
			return Http2::ErrorCode::FRAME_SIZE_ERROR;
		}

		const Http2::ErrorCode error_code = static_cast<const Http2::ErrorCode>(
			::ntohl(*reinterpret_cast<const uint32_t *>(src) )
		);

		if (Http2::ErrorCode::NO_ERROR != error_code) {
			// DEBUG
		}

		return Http2::ErrorCode::NO_ERROR;
	}

	static Http2::ErrorCode parseHttp2Settings(
		Http2::FrameMeta &meta,
		Http2::IncStream &stream,
		const uint8_t *src,
		const uint8_t *end
	) {
		if (0 != meta.stream_id) {
			return Http2::ErrorCode::PROTOCOL_ERROR;
		}

		if (meta.length % (sizeof(uint16_t) + sizeof(uint32_t) ) ) {
			return Http2::ErrorCode::FRAME_SIZE_ERROR;
		}

		if (Http2::StreamState::OPEN != stream.state) {
			stream.state = Http2::StreamState::OPEN;
		}

		Http2::ConnectionSettings &settings = stream.conn.client_settings;

		while (src != end)
		{
			const Http2::ConnectionSetting setting = static_cast<Http2::ConnectionSetting>(
				ntohs(*reinterpret_cast<const uint16_t *>(src) )
			);

			src += sizeof(uint16_t);

			const uint32_t value = ::ntohl(*reinterpret_cast<const uint32_t *>(src) );

			src += sizeof(uint32_t);

			switch (setting)
			{
				case Http2::ConnectionSetting::SETTINGS_HEADER_TABLE_SIZE: {
					settings.header_table_size = value;
					break;
				}

				case Http2::ConnectionSetting::SETTINGS_ENABLE_PUSH: {
					if (value > 1) {
						return Http2::ErrorCode::PROTOCOL_ERROR;
					}

					settings.enable_push = value;

					break;
				}

				case Http2::ConnectionSetting::SETTINGS_MAX_CONCURRENT_STREAMS: {
					settings.max_concurrent_streams = value;
					break;
				}

				case Http2::ConnectionSetting::SETTINGS_INITIAL_WINDOW_SIZE: {
					if (value >= uint32_t(1) << 31) {
						return Http2::ErrorCode::FLOW_CONTROL_ERROR;
					}

					settings.initial_window_size = value;

					break;
				}

				case Http2::ConnectionSetting::SETTINGS_MAX_FRAME_SIZE: {
					if (value < (1 << 14) || value >= (1 << 24) ) {
						return Http2::ErrorCode::PROTOCOL_ERROR;
					}

					settings.max_frame_size = value;

					break;
				}

				case Http2::ConnectionSetting::SETTINGS_MAX_HEADER_LIST_SIZE: {
					settings.max_header_list_size = value;
					break;
				}

				default:
					break;
			}
		}

		return Http2::ErrorCode::NO_ERROR;
	}

	static Http2::ErrorCode parseHttp2GoAway(
		Http2::FrameMeta &meta,
		Http2::IncStream &stream,
		const uint8_t *src,
		const uint8_t *end
	) {
		if (0 != meta.stream_id) {
			return Http2::ErrorCode::PROTOCOL_ERROR;
		}

		stream.state = Http2::StreamState::CLOSED;

		const uint32_t last_stream_id = ::ntohl(
			*reinterpret_cast<const uint32_t *>(src)
		);

		if (last_stream_id > 0) {

		}

		src += sizeof(uint32_t);

		const Http2::ErrorCode error_code = static_cast<Http2::ErrorCode>(
			::ntohl(*reinterpret_cast<const uint32_t *>(src) )
		);

		if (Http2::ErrorCode::NO_ERROR != error_code) {

		}

		return Http2::ErrorCode::NO_ERROR;
	}

	static void ping(
		const Socket::Adapter &sock,
		const std::chrono::milliseconds &timeout,
		Http2::ConnectionData &conn,
		const uint64_t pingData
	) {
		constexpr uint32_t frame_size = sizeof(uint64_t);

		std::array<uint8_t, Http2::FRAME_HEADER_SIZE + frame_size> buf;
		uint8_t *addr = buf.data();

		constexpr uint32_t stream_id = 0;

		addr = setHttp2FrameHeader(
			addr,
			frame_size,
			Http2::FrameType::PING,
			Http2::FrameFlag::ACK,
			stream_id
		);

		*reinterpret_cast<uint64_t *>(addr) = pingData;

		const std::unique_lock<std::mutex> lock(conn.sync.mtx);

		sock.nonblock_send(buf.data(), buf.size(), timeout);
	}

	static Http2::ErrorCode parseHttp2Ping(
		Http2::FrameMeta &meta
	) {
		if (0 != meta.stream_id) {
			return Http2::ErrorCode::PROTOCOL_ERROR;
		}

		if (sizeof(uint64_t) != meta.length) {
			return Http2::ErrorCode::FRAME_SIZE_ERROR;
		}

		return Http2::ErrorCode::NO_ERROR;
	}

	static Http2::ErrorCode parseHttp2WindowUpdate(
		Http2::FrameMeta &meta,
		Http2::IncStream &stream,
		const uint8_t *src,
		const uint8_t *end
	) {
		if (Http2::StreamState::RESERVED == stream.state) {
			return Http2::ErrorCode::PROTOCOL_ERROR;
		}
		else if (Http2::StreamState::OPEN != stream.state) {
			return Http2::ErrorCode::NO_ERROR;
		}

		if (sizeof(uint32_t) != meta.length) {
			return Http2::ErrorCode::FRAME_SIZE_ERROR;
		}

		const uint32_t window_size_increment = ::ntohl(
			*reinterpret_cast<const uint32_t *>(src)
		);

		if (0 == window_size_increment) {
			return Http2::ErrorCode::PROTOCOL_ERROR;
		}
		else if (window_size_increment >= uint32_t(1) << 31) {
			return Http2::ErrorCode::FLOW_CONTROL_ERROR;
		}

		if (0 == meta.stream_id) {
			// TODO: update all streams
			stream.window_size_out += window_size_increment;
		} else {
			stream.window_size_out += window_size_increment;
		}

		return Http2::ErrorCode::NO_ERROR;
	}

	static void rstStream(
		const Socket::Adapter &sock,
		const std::chrono::milliseconds &timeout,
		Http2::IncStream &stream,
		const Http2::ErrorCode errorCode
	) {
		constexpr uint32_t frame_size = sizeof(uint32_t);

		std::array<uint8_t, Http2::FRAME_HEADER_SIZE + frame_size> buf;
		uint8_t *addr = buf.data();

		addr = setHttp2FrameHeader(
			addr,
			frame_size,
			Http2::FrameType::RST_STREAM,
			Http2::FrameFlag::EMPTY,
			stream.stream_id
		);

		*reinterpret_cast<uint32_t *>(addr) = ::htonl(
			static_cast<const uint32_t>(errorCode)
		);

		const std::unique_lock<std::mutex> lock(stream.conn.sync.mtx);

		sock.nonblock_send(buf.data(), buf.size(), timeout);
	}

	static void sendSettings(
		const Socket::Adapter &sock,
		const std::chrono::milliseconds &timeout,
		Http2::ConnectionData &conn,
		const uint8_t *src,
		const uint8_t *end
	) {
		const uint32_t frame_size = uint32_t(end - src);

		std::vector<uint8_t> buf(Http2::FRAME_HEADER_SIZE + frame_size);

		uint8_t *addr = buf.data();

		constexpr uint32_t stream_id = 0;

		addr = setHttp2FrameHeader(
			addr,
			frame_size,
			Http2::FrameType::SETTINGS,
			Http2::FrameFlag::EMPTY,
			stream_id
		);

		std::copy(src, end, addr);

		const std::unique_lock<std::mutex> lock(conn.sync.mtx);

		sock.nonblock_send(buf.data(), buf.size(), timeout);
	}

	static void goAway(
		const Socket::Adapter &sock,
		const std::chrono::milliseconds &timeout,
		Http2::ConnectionData &conn,
		const uint32_t lastStreamId,
		const Http2::ErrorCode errorCode
	) {
		constexpr uint32_t frame_size = sizeof(uint32_t) * 2;

		std::array<uint8_t, Http2::FRAME_HEADER_SIZE + frame_size> buf;

		uint8_t *addr = buf.data();

		addr = setHttp2FrameHeader(
			addr,
			frame_size,
			Http2::FrameType::RST_STREAM,
			Http2::FrameFlag::EMPTY,
			0
		);

		*reinterpret_cast<uint32_t *>(addr) = ::htonl(lastStreamId);

		*reinterpret_cast<uint32_t *>(addr + sizeof(uint32_t) ) = ::htonl(
			static_cast<const uint32_t>(errorCode)
		);

		const std::unique_lock<std::mutex> lock(conn.sync.mtx);

		sock.nonblock_send(buf.data(), buf.size(), timeout);
	}

	static bool getClientPreface(
		const Socket::Adapter &sock,
		const std::chrono::milliseconds &timeout
	) {
		std::array<uint8_t, 24> buf;

		const long read_size = sock.nonblock_recv(
			buf.data(),
			buf.size(),
			timeout
		);

		if (buf.size() != read_size) {
			return false;
		}

		static constexpr char client_preface_data[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

		const uint64_t *left = reinterpret_cast<const uint64_t *>(client_preface_data);
		const uint64_t *right = reinterpret_cast<const uint64_t *>(buf.data() );

		uint64_t compare = 0;

		compare |= left[0] ^ right[0];
		compare |= left[1] ^ right[1];
		compare |= left[2] ^ right[2];

		return 0 == compare;
	}

	static void sendEmptySettings(
		const Socket::Adapter &sock,
		const std::chrono::milliseconds &timeout,
		Http2::ConnectionData &conn,
		const Http2::FrameFlag flags
	) {
		constexpr uint32_t frame_size = 0;

		std::array<uint8_t, Http2::FRAME_HEADER_SIZE + frame_size> buf;
		uint8_t *addr = buf.data();

		constexpr uint32_t stream_id = 0;

		addr = setHttp2FrameHeader(
			addr,
			frame_size,
			Http2::FrameType::SETTINGS,
			flags,
			stream_id
		);

		const std::unique_lock<std::mutex> lock(conn.sync.mtx);

		sock.nonblock_send(buf.data(), buf.size(), timeout);
	}

	static bool getNextHttp2FrameMeta(
		const Socket::Adapter &sock,
		const std::chrono::milliseconds &timeout,
		std::vector<char> &buf,
		Http2::FrameMeta &meta,
		long &read_size
	) {
		const long length = long(
			meta.length + Http2::FRAME_HEADER_SIZE
		);

		if (read_size <= length) {
			if (read_size == length) {
				read_size = 0;
			}

			read_size = sock.nonblock_recv(
				buf.data() + read_size,
				buf.size() - size_t(read_size),
				timeout
			);

			if (read_size < long(Http2::FRAME_HEADER_SIZE) ) {
				return false;
			}
		} else {
			std::copy(
				buf.data() + length,
				buf.data() + read_size,
				buf.data()
			);

			read_size -= length;
		}

		const uint8_t *addr = reinterpret_cast<const uint8_t *>(buf.data() );

		meta.length = Utils::ntoh24(addr);
		meta.type = static_cast<Http2::FrameType>(*(addr + 3) );
		meta.flags = static_cast<Http2::FrameFlag>(*(addr + 4) );
		meta.stream_id = ::ntohl(*reinterpret_cast<const uint32_t *>(addr + 5) );

		return true;
	}

	ServerProtocol *ServerHttp2::process()
	{
		struct Request req;
		req.timeout = std::chrono::milliseconds(15000);
		req.protocol_variant = Transfer::ProtocolVariant::HTTP_2;

		Http2::ConnectionData conn;

		sendEmptySettings(
			this->sock,
			req.timeout,
			conn,
			Http2::FrameFlag::EMPTY
		);

		if (getClientPreface(this->sock, req.timeout) == false) {
			constexpr uint32_t last_stream_id = 0;

			goAway(
				this->sock,
				req.timeout,
				conn,
				last_stream_id,
				Http2::ErrorCode::PROTOCOL_ERROR
			);

			return this;
		}

		conn.client_settings = Http2::ConnectionSettings::defaultSettings();
		conn.server_settings = Http2::ConnectionSettings::defaultSettings();

		std::vector<char> buf(conn.server_settings.max_frame_size);

		std::unordered_map<uint32_t, Http2::IncStream> streams {
			{ 0, Http2::IncStream(0, conn) }
		};

		Http2::IncStream &primary = streams.find(0)->second;
		this->stream = &primary;

		size_t streams_process_count = 0;
		uint32_t last_stream_id = 0;

		Http2::FrameMeta meta {};
		long read_size = 0;

		do {
			if (getNextHttp2FrameMeta(this->sock, req.timeout, buf, meta, read_size) == false) {
				break;
			}

			const uint8_t *addr = reinterpret_cast<const uint8_t *>(
				buf.data()
			) + Http2::FRAME_HEADER_SIZE;

			const uint8_t *end = addr + meta.length;

			if (meta.stream_id > last_stream_id) {
				last_stream_id = meta.stream_id;
			}

			Http2::IncStream &stream = getStreamData(streams, meta.stream_id, conn);

			if (Http2::StreamState::CLOSED == stream.state) {
				rstStream(this->sock, req.timeout, stream, Http2::ErrorCode::STREAM_CLOSED);
				continue;
			}

			if (meta.type != Http2::FrameType::CONTINUATION) {
				stream.frame_type = meta.type;
			}

			Http2::ErrorCode result = Http2::ErrorCode::NO_ERROR;

			switch (stream.frame_type)
			{
			case Http2::FrameType::DATA:
			{
				result = parseHttp2Data(meta, stream, addr, end);

				stream.window_size_inc -= meta.length;

				if (stream.reserved)
				{
					DataVariant::DataReceiver *dr = reinterpret_cast<DataVariant::DataReceiver *>(stream.reserved);

					if (stream.window_size_inc - long(conn.server_settings.max_frame_size) <= 0)
					{
						size_t update_size = conn.server_settings.initial_window_size +
							(dr->full_size - dr->recv_total) - size_t(stream.window_size_inc);

						if (update_size > Http2::MAX_WINDOW_UPDATE) {
							update_size = Http2::MAX_WINDOW_UPDATE;
						}

						sendWindowUpdate(this->sock, req.timeout, stream, uint32_t(update_size) );
						sendWindowUpdate(this->sock, req.timeout, primary, uint32_t(update_size) );

						stream.window_size_inc += update_size;
					}
				}

				break;
			}

			case Http2::FrameType::HEADERS:
			{
				result = parseHttp2Headers(meta, stream, addr, end);

				if (meta.flags & Http2::FrameFlag::END_HEADERS)
				{
					Transfer::request_data *rd = static_cast<Transfer::request_data *>(&stream);

					stream.reserved = createDataReceiver(rd, this->settings.variants);

					if (stream.reserved) {
						DataVariant::DataReceiver *dr = reinterpret_cast<DataVariant::DataReceiver *>(stream.reserved);
						dr->reserved = new std::string();
					}
				}

				break;
			}

			case Http2::FrameType::PRIORITY:
				result = Http2::ErrorCode::NO_ERROR;
				break;

			case Http2::FrameType::RST_STREAM:
				result = parseHttp2rstStream(meta, stream, addr, end);
				break;

			case Http2::FrameType::SETTINGS:
			{
				result = parseHttp2Settings(meta, stream, addr, end);

				if (Http2::ErrorCode::NO_ERROR == result && (meta.flags & Http2::FrameFlag::ACK) == false)
				{
					conn.decoding_dynamic_table.changeHeaderTableSize(conn.client_settings.header_table_size);
					conn.decoding_dynamic_table.changeMaxHeaderListSize(conn.client_settings.max_header_list_size);

					sendEmptySettings(this->sock, req.timeout, conn, Http2::FrameFlag::ACK);
				}

				break;
			}

			case Http2::FrameType::PUSH_PROMISE:
				result = Http2::ErrorCode::NO_ERROR;
				break;

			case Http2::FrameType::PING:
			{
				result = parseHttp2Ping(meta);

				if (Http2::ErrorCode::NO_ERROR == result && (meta.flags & Http2::FrameFlag::ACK) == false)
				{
					const uint64_t ping_data = *reinterpret_cast<const uint64_t *>(addr);
					ping(this->sock, req.timeout, conn, ping_data);
				}

				break;
			}

			case Http2::FrameType::GOAWAY:
				result = parseHttp2GoAway(meta, stream, addr, end);
				break;

			case Http2::FrameType::WINDOW_UPDATE:
				result = parseHttp2WindowUpdate(meta, stream, addr, end);
				break;

			default:
				result = Http2::ErrorCode::PROTOCOL_ERROR;
				break;
			}

			if (result != Http2::ErrorCode::NO_ERROR)
			{
				stream.state = Http2::StreamState::CLOSED;

				rstStream(this->sock, req.timeout, stream, result);

				// TODO: remove closed stream(s) from unordered map
			}
			else if ( (meta.flags & Http2::FrameFlag::END_STREAM) && meta.stream_id != 0)
			{
				stream.reserved = this->sock.get_tls_session();

				sockets.lock();

				sockets.emplace(
					std::tuple<Socket::Socket, Http2::IncStream *> {
						Socket::Socket(this->sock.get_handle() ),
						&stream
					}
				);

				sockets.unlock();

				this->controls.eventProcessQueue->notify();

				++streams_process_count;
			}
		}
		while (Http2::StreamState::CLOSED != primary.state);

		while (conn.sync.completed.load() < streams_process_count) {
			conn.sync.event.wait();
		}

		goAway(
			this->sock,
			req.timeout,
			conn,
			last_stream_id,
			Http2::ErrorCode::NO_ERROR
		);

		for (auto &pair : streams) {
			destroyDataReceiver(pair.second.reserved);
		}

		return this;
	}
}
