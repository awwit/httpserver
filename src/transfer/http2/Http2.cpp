
#include "Http2.h"
#include "../../utils/Utils.h"

namespace Http2
{
	bool operator &(
		const FrameFlag left,
		const FrameFlag right
	) noexcept {
		return static_cast<const uint8_t>(left) & static_cast<const uint8_t>(right);
	}

	FrameFlag operator |(
		const FrameFlag left,
		const FrameFlag right
	) noexcept {
		return static_cast<FrameFlag>(
			static_cast<const uint8_t>(left) | static_cast<const uint8_t>(right)
		);
	}

	FrameFlag operator |=(
		FrameFlag &left,
		const FrameFlag right
	) noexcept {
		return static_cast<FrameFlag>(
			*reinterpret_cast<uint8_t *>(&left) |= static_cast<const uint8_t>(right)
		);
	}

	ConnectionSettings ConnectionSettings::defaultSettings() noexcept {
		return ConnectionSettings {
			4096,
			1,
			0,
			(1 << 16) - 1, // 65535
			1 << 14, // 16384
			0
		};
	}

	DynamicTable::DynamicTable() noexcept
		: header_table_size(0),
		  max_header_list_size(0),
		  cur_header_list_size(0)
	{

	}

	size_t DynamicTable::size() const noexcept {
		return this->list.size();
	}

	DynamicTable::DynamicTable(
		const uint32_t headerTableSize,
		const uint32_t maxHeaderListSize,
		std::deque<std::pair<std::string, std::string> > &&list
	) noexcept
		: list(std::move(list) ),
		  header_table_size(headerTableSize),
		  max_header_list_size(maxHeaderListSize),
		  cur_header_list_size(0)
	{
		for (auto const &pair : list) {
			this->cur_header_list_size += pair.first.length() + pair.second.length();
		}
	}

	void DynamicTable::addHeader(const std::pair<std::string, std::string> &header)
	{
		this->cur_header_list_size += header.first.length() + header.second.length();

		this->list.emplace_front(header);

		while (
			this->list.size() > this->header_table_size || (
				this->max_header_list_size != 0 &&
				this->cur_header_list_size > this->max_header_list_size
			)
		) {
			auto const &pair = this->list.back();

			this->cur_header_list_size -= pair.first.length() + pair.second.length();

			this->list.pop_back();
		}
	}

	void DynamicTable::addHeader(std::pair<std::string, std::string> &&header)
	{
		this->cur_header_list_size += header.first.length() + header.second.length();

		this->list.emplace_front(std::move(header) );

		while (
			this->list.size() > this->header_table_size || (
				this->max_header_list_size != 0 &&
				this->cur_header_list_size > this->max_header_list_size
			)
		) {
			auto const &pair = this->list.back();

			this->cur_header_list_size -= pair.first.length() + pair.second.length();

			this->list.pop_back();
		}
	}

	void DynamicTable::changeHeaderTableSize(const uint32_t headerTableSize)
	{
		this->header_table_size = headerTableSize;

		while (this->list.size() > this->header_table_size) {
			auto const &pair = this->list.back();

			this->cur_header_list_size -= pair.first.length() + pair.second.length();

			this->list.pop_back();
		}
	}

	void DynamicTable::changeMaxHeaderListSize(const uint32_t maxHeaderListSize)
	{
		this->max_header_list_size = maxHeaderListSize;

		while (
			this->max_header_list_size != 0 &&
			this->cur_header_list_size > this->max_header_list_size
		) {
			auto const &pair = this->list.back();

			this->cur_header_list_size -= pair.first.length() + pair.second.length();

			this->list.pop_back();
		}
	}

	const std::pair<std::string, std::string> &
	DynamicTable::operator[](const size_t index) const noexcept {
		return this->list[index];
	}

	std::pair<std::string, std::string> &
	DynamicTable::operator[](const size_t index) noexcept {
		return this->list[index];
	}

	const std::deque<std::pair<std::string, std::string> > &DynamicTable::getList() const noexcept {
		return this->list;
	}

	IncStream::IncStream(const uint32_t streamId, ConnectionData &conn) noexcept
		: conn(conn),
		  window_size_inc(int32_t(conn.server_settings.initial_window_size)),
		  window_size_out(int32_t(conn.client_settings.initial_window_size)),
		  stream_id(streamId),
		  state(StreamState::IDLE),
		  priority(0),
		  reserved(nullptr)
	{

	}

	uint8_t *IncStream::setHttp2FrameHeader(
		uint8_t *addr,
		const uint32_t frameSize,
		const Http2::FrameType frameType,
		const Http2::FrameFlag frameFlags
	) noexcept {
		Utils::hton24(addr, frameSize);
		*(addr + 3) = static_cast<const uint8_t>(frameType);
		*(addr + 4) = static_cast<const uint8_t>(frameFlags);
		*reinterpret_cast<uint32_t *>(addr + 5) = ::htonl(this->stream_id);

		return (addr + Http2::FRAME_HEADER_SIZE);
	}

	void IncStream::lock() {
		this->conn.sync.mtx.lock();
	}

	void IncStream::unlock() noexcept {
		this->conn.sync.mtx.unlock();
	}

	void IncStream::close() noexcept {
		this->incoming_headers.clear();
		this->incoming_data.clear();
		this->incoming_files.clear();
		this->reserved = nullptr;

		++this->conn.sync.completed;
		this->conn.sync.event.notify();

	//	this->state = StreamState::CLOSED;
	}

	OutStream::OutStream(
		const uint32_t streamId,
		const ConnectionSettings &settings,
		DynamicTable &&dynamic_table,
		std::mutex *mtx
	) noexcept
		: stream_id(streamId),
		  window_size_out(int32_t(settings.initial_window_size)),
		  settings(settings),
		  dynamic_table(std::move(dynamic_table) ),
		  mtx(mtx)
	{

	}

	OutStream::OutStream(const IncStream &stream)
		: stream_id(stream.stream_id),
		  window_size_out(stream.window_size_out),
		  settings(stream.conn.client_settings),
		  dynamic_table(stream.conn.encoding_dynamic_table),
		  mtx(&stream.conn.sync.mtx)
	{

	}

	uint8_t *OutStream::setHttp2FrameHeader(
		uint8_t *addr,
		const uint32_t frameSize,
		const Http2::FrameType frameType,
		const Http2::FrameFlag frameFlags
	) noexcept {
		Utils::hton24(addr, frameSize);
		*(addr + 3) = static_cast<const uint8_t>(frameType);
		*(addr + 4) = static_cast<const uint8_t>(frameFlags);
		*reinterpret_cast<uint32_t *>(addr + 5) = ::htonl(this->stream_id);

		return (addr + Http2::FRAME_HEADER_SIZE);
	}

	void OutStream::lock() {
		this->mtx->lock();
	}

	void OutStream::unlock() noexcept {
		this->mtx->unlock();
	}
}
