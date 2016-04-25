#pragma once

#include "SocketAdapter.h"
#include "Socket.h"

namespace HttpServer
{
	class SocketAdapterDefault : public SocketAdapter
	{
	private:
		Socket sock;

	public:
		SocketAdapterDefault() = delete;
		SocketAdapterDefault(const Socket &_sock);

		virtual System::native_socket_type get_handle() const override;
		virtual ::gnutls_session_t get_tls_session() const override;
		virtual SocketAdapter *copy() const override;

		virtual long nonblock_recv(std::vector<std::string::value_type> &buf, const std::chrono::milliseconds &timeout) const override;

		virtual long nonblock_send(const std::string &buf, const std::chrono::milliseconds &timeout) const override;
		virtual long nonblock_send(const std::vector<std::string::value_type> &buf, const size_t length, const std::chrono::milliseconds &timeout) const override;

		virtual void close() override;
	};
};