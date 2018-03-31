#pragma once

#include "Adapter.h"
#include "Socket.h"

namespace Socket
{
	class AdapterDefault : public Adapter
	{
	private:
		Socket sock;

	public:
		AdapterDefault() = delete;
		AdapterDefault(const Socket &_sock) noexcept;

		virtual System::native_socket_type get_handle() const noexcept override;
		virtual ::gnutls_session_t get_tls_session() const noexcept override;
		virtual Adapter *copy() const noexcept override;

		virtual long nonblock_recv(
			void *buf,
			const size_t length,
			const std::chrono::milliseconds &timeout
		) const noexcept override;

		virtual long nonblock_send(
			const void *buf,
			const size_t length,
			const std::chrono::milliseconds &timeout
		) const noexcept override;

		virtual void close() noexcept override;
	};
}
