
#include "SocketAdapterTls.h"

namespace HttpServer
{
	SocketAdapterTls::SocketAdapterTls(const Socket &sock, ::gnutls_priority_t priority_cache, ::gnutls_certificate_credentials_t x509_cred)
	{
		::gnutls_init(&this->session, GNUTLS_SERVER);
		::gnutls_priority_set(this->session, priority_cache);
		::gnutls_credentials_set(this->session, GNUTLS_CRD_CERTIFICATE, x509_cred);

		::gnutls_certificate_server_set_request(this->session, GNUTLS_CERT_IGNORE);

		::gnutls_transport_set_int2(this->session, sock.get_handle(), sock.get_handle() );
	}

	SocketAdapterTls::SocketAdapterTls(const ::gnutls_session_t _session) : session(_session)
	{

	}

	bool SocketAdapterTls::handshake()
	{
		int ret;

		do
		{
			ret = ::gnutls_handshake(this->session);
		}
		while (ret < 0 && ::gnutls_error_is_fatal(ret) == 0);

		if (ret < 0)
		{
			Socket sock(this->get_handle() );

			sock.close();
			::gnutls_deinit(this->session);

			return false;
		}

		return true;
	}

	long SocketAdapterTls::nonblock_send_all(const void *buf, const size_t length, const std::chrono::milliseconds &timeout) const
	{
		size_t record_size = ::gnutls_record_get_max_size(this->session);

		if (0 == record_size)
		{
			return -1;
		}

		size_t total = 0;

		while (total < length)
		{
			::gnutls_record_set_timeout(this->session, timeout.count() );

			if (record_size > length - total)
			{
				record_size = length - total;
			}

			const long send_size = ::gnutls_record_send(this->session, reinterpret_cast<const uint8_t *>(buf) + total, record_size);

			if (send_size < 0)
			{
				return send_size;
			}

			total += send_size;
		}

		return total;
	}

	System::native_socket_type SocketAdapterTls::get_handle() const
	{
		return reinterpret_cast<System::native_socket_type>(::gnutls_transport_get_int(this->session) );
	}

	::gnutls_session_t SocketAdapterTls::get_tls_session() const
	{
		return this->session;
	}

	SocketAdapter *SocketAdapterTls::copy() const
	{
		return new SocketAdapterTls(this->session);
	}

	long SocketAdapterTls::nonblock_recv(std::vector<std::string::value_type> &buf, const std::chrono::milliseconds &timeout) const
	{
		::gnutls_record_set_timeout(this->session, timeout.count() );
		return ::gnutls_record_recv(this->session, buf.data(), buf.size() );
	}

	long SocketAdapterTls::nonblock_send(const std::string &buf, const std::chrono::milliseconds &timeout) const
	{
		return this->nonblock_send_all(buf.data(), buf.length(), timeout);
	}

	long SocketAdapterTls::nonblock_send(const std::vector<std::string::value_type> &buf, const size_t length, const std::chrono::milliseconds &timeout) const
	{
		return this->nonblock_send_all(buf.data(), length, timeout);
	}

	void SocketAdapterTls::close()
	{
		Socket sock(this->get_handle() );

		// Wait for send all data to client
		sock.nonblock_send_sync();

		::gnutls_bye(this->session, GNUTLS_SHUT_RDWR);

		sock.close();

		::gnutls_deinit(this->session);
	}
};