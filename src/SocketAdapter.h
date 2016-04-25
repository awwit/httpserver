#pragma once

#include "System.h"

#include <vector>
#include <string>
#include <chrono>

#include <gnutls/gnutls.h>

namespace HttpServer
{
	class SocketAdapter
	{
	public:
		virtual ~SocketAdapter() = default;

		virtual System::native_socket_type get_handle() const = 0;
		virtual ::gnutls_session_t get_tls_session() const = 0;
		virtual SocketAdapter *copy() const = 0;

		virtual long nonblock_recv(std::vector<std::string::value_type> &buf, const std::chrono::milliseconds &timeout) const = 0;

		virtual long nonblock_send(const std::string &buf, const std::chrono::milliseconds &timeout) const = 0;
		virtual long nonblock_send(const std::vector<std::string::value_type> &buf, const size_t length, const std::chrono::milliseconds &timeout) const = 0;

		virtual void close() = 0;

		bool operator ==(const SocketAdapter &obj) const;
		bool operator !=(const SocketAdapter &obj) const;
	};
};