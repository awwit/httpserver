#pragma once

#include "Adapter.h"
#include "Socket.h"

namespace Socket
{
	class AdapterTls : public Adapter
	{
	private:
		::gnutls_session_t session;

	public:
		AdapterTls() = delete;

		AdapterTls(
			const Socket &sock,
			::gnutls_priority_t priority_cache,
			::gnutls_certificate_credentials_t x509_cred
		) noexcept;

		AdapterTls(const ::gnutls_session_t session) noexcept;

		bool handshake() noexcept;

		long nonblock_send_all(
			const void *buf,
			const size_t length,
			const std::chrono::milliseconds &timeout
		) const noexcept;

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
