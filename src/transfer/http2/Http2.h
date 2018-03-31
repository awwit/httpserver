#pragma once

#include "../../utils/Event.h"
#include "../AppRequest.h"

#include <deque>
#include <utility>
#include <string>
#include <unordered_map>
#include <cstddef>

#ifdef WIN32
	#undef NO_ERROR
#elif POSIX
	#include <arpa/inet.h>
#endif

namespace Http2
{
	enum class ErrorCode : uint32_t {
		NO_ERROR = 0x0,
		PROTOCOL_ERROR,
		INTERNAL_ERROR,
		FLOW_CONTROL_ERROR,
		SETTINGS_TIMEOUT,
		STREAM_CLOSED,
		FRAME_SIZE_ERROR,
		REFUSED_STREAM,
		CANCEL,
		COMPRESSION_ERROR,
		CONNECT_ERROR,
		ENHANCE_YOUR_CALM,
		INADEQUATE_SECURITY,
		HTTP_1_1_REQUIRED
	};

	enum class FrameType : uint8_t {
		DATA = 0x0,
		HEADERS,
		PRIORITY,
		RST_STREAM,
		SETTINGS,
		PUSH_PROMISE,
		PING,
		GOAWAY,
		WINDOW_UPDATE,
		CONTINUATION
	};

	enum class FrameFlag : uint8_t {
		EMPTY = 0x0,
		ACK = 0x1,
		END_STREAM = 0x1,
		END_HEADERS = 0x4,
		PADDED = 0x8,
		PRIORITY = 0x20
	};

	bool operator &(const FrameFlag left, const FrameFlag right) noexcept;
	FrameFlag operator |(const FrameFlag left, const FrameFlag right) noexcept;
	FrameFlag operator |=(FrameFlag &left, const FrameFlag right) noexcept;

	struct FrameMeta {
		uint32_t stream_id;
		uint32_t length;
		FrameType type;
		FrameFlag flags;
	};

	constexpr uint32_t FRAME_HEADER_SIZE = 9;
	constexpr uint32_t MAX_WINDOW_UPDATE = (uint32_t(1) << 31) - 1;

	enum class ConnectionSetting : uint16_t {
		SETTINGS_HEADER_TABLE_SIZE = 0x1,
		SETTINGS_ENABLE_PUSH = 0x2,
		SETTINGS_MAX_CONCURRENT_STREAMS = 0x3,
		SETTINGS_INITIAL_WINDOW_SIZE = 0x4,
		SETTINGS_MAX_FRAME_SIZE = 0x5,
		SETTINGS_MAX_HEADER_LIST_SIZE = 0x6
	};

	struct ConnectionSettings {
		uint32_t header_table_size;
		uint32_t enable_push;
		uint32_t max_concurrent_streams;
		uint32_t initial_window_size;
		uint32_t max_frame_size;
		uint32_t max_header_list_size;

		static ConnectionSettings defaultSettings() noexcept;
	};

	enum class StreamState : uint8_t {
		IDLE,
		RESERVED,
		OPEN,
		HALF_CLOSED,
		CLOSED
	};

	struct ConnectionSync {
		Utils::Event event;
		std::mutex mtx;
		std::atomic<std::size_t> completed {};
	};

	class DynamicTable
	{
	private:
		std::deque<std::pair<std::string, std::string> > list;

		uint32_t header_table_size;
		uint32_t max_header_list_size;

		uint32_t cur_header_list_size;

	public:
		DynamicTable() noexcept;

		DynamicTable(
			const uint32_t headerTableSize,
			const uint32_t maxHeaderListSize,
			std::deque<std::pair<std::string, std::string> > &&list
		) noexcept;

		size_t size() const noexcept;

		void addHeader(const std::pair<std::string, std::string> &header);
		void addHeader(std::pair<std::string, std::string> &&header);

		void changeHeaderTableSize(const uint32_t headerTableSize);
		void changeMaxHeaderListSize(const uint32_t maxHeaderListSize);

		const std::pair<std::string, std::string> &
		operator[](const size_t index) const noexcept;

		std::pair<std::string, std::string> &
		operator[](const size_t index) noexcept;

		const std::deque<std::pair<std::string, std::string> > &
		getList() const noexcept;
	};

	struct ConnectionData {
		DynamicTable decoding_dynamic_table;
		DynamicTable encoding_dynamic_table;

		ConnectionSettings client_settings;
		ConnectionSettings server_settings;

		ConnectionSync sync;
	};

	class IncStream : public Transfer::request_data
	{
	public:
		ConnectionData &conn;

		int32_t window_size_inc;
		int32_t window_size_out;

		uint32_t stream_id;

		StreamState state;
		uint8_t priority;

		FrameType frame_type;

		void *reserved;

	public:
		IncStream(
			const uint32_t streamId,
			ConnectionData &conn
		) noexcept;

		uint8_t *setHttp2FrameHeader(
			uint8_t *addr,
			const uint32_t frameSize,
			const Http2::FrameType frameType,
			const Http2::FrameFlag frameFlags
		) noexcept;

		void lock();
		void unlock() noexcept;

		void close() noexcept;
	};

	struct OutStream
	{
		uint32_t stream_id;
		int32_t window_size_out;

		ConnectionSettings settings;
		DynamicTable dynamic_table;

		std::mutex *mtx;

	public:
		OutStream(
			const uint32_t streamId,
			const ConnectionSettings &settings,
			DynamicTable &&dynamic_table,
			std::mutex *mtx
		) noexcept;

		OutStream(const IncStream &stream);

		uint8_t *setHttp2FrameHeader(
			uint8_t *addr,
			const uint32_t frameSize,
			const Http2::FrameType frameType,
			const Http2::FrameFlag frameFlags
		) noexcept;

		void lock();
		void unlock() noexcept;
	};
}
