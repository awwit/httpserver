
#include "AdapterDefault.h"

namespace Socket
{
	AdapterDefault::AdapterDefault(const Socket &sock) noexcept : sock(sock) {}

	System::native_socket_type AdapterDefault::get_handle() const noexcept {
		return sock.get_handle();
	}

	::gnutls_session_t AdapterDefault::get_tls_session() const noexcept {
		return nullptr;
	}

	Adapter *AdapterDefault::copy() const noexcept {
		return new AdapterDefault(this->sock);
	}

	long AdapterDefault::nonblock_recv(
		void *buf,
		const size_t length,
		const std::chrono::milliseconds &timeout
	) const noexcept {
		return sock.nonblock_recv(buf, length, timeout);
	}

	long AdapterDefault::nonblock_send(
		const void *buf,
		const size_t length,
		const std::chrono::milliseconds &timeout
	) const noexcept {
		return sock.nonblock_send(buf, length, timeout);
	}

	void AdapterDefault::close() noexcept {
		// Wait for send all data to client
		sock.nonblock_send_sync();

		sock.shutdown();
		sock.close();
	}
}
