#pragma once

#include "SocketAdapter.h"
#include "Socket.h"

namespace HttpServer
{
	class SocketAdapterTls : public SocketAdapter
	{
	private:
		::gnutls_session_t session;

	public:
		SocketAdapterTls() = delete;
		SocketAdapterTls(const Socket &sock, ::gnutls_priority_t priority_cache, ::gnutls_certificate_credentials_t x509_cred);
		SocketAdapterTls(const ::gnutls_session_t _session);

		bool handshake();
		long nonblock_send_all(const void *buf, const size_t length, const std::chrono::milliseconds &timeout) const;

		virtual System::native_socket_type get_handle() const override;
		virtual ::gnutls_session_t get_tls_session() const override;
		virtual SocketAdapter *copy() const override;

		virtual long nonblock_recv(std::vector<std::string::value_type> &buf, const std::chrono::milliseconds &timeout) const override;

		virtual long nonblock_send(const std::string &buf, const std::chrono::milliseconds &timeout) const override;
		virtual long nonblock_send(const std::vector<std::string::value_type> &buf, const size_t length, const std::chrono::milliseconds &timeout) const override;

		virtual void close() override;
	};
};