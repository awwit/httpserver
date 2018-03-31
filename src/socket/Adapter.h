#pragma once

#include "../system/System.h"

#include <vector>
#include <string>
#include <chrono>

#include <gnutls/gnutls.h>

namespace Socket
{
	class Adapter
	{
	public:
		virtual ~Adapter() noexcept = default;

		virtual System::native_socket_type get_handle() const noexcept = 0;
		virtual ::gnutls_session_t get_tls_session() const noexcept = 0;
		virtual Adapter *copy() const noexcept = 0;

		long nonblock_recv(
			std::vector<std::string::value_type> &buf,
			const std::chrono::milliseconds &timeout
		) const noexcept;

		virtual long nonblock_recv(
			void *buf,
			const size_t length,
			const std::chrono::milliseconds &timeout
		) const noexcept = 0;

		long nonblock_send(
			const std::string &buf,
			const std::chrono::milliseconds &timeout
		) const noexcept;

		virtual long nonblock_send(
			const void *buf,
			const size_t length,
			const std::chrono::milliseconds &timeout
		) const noexcept = 0;

		virtual void close() noexcept = 0;

		bool operator ==(const Adapter &obj) const noexcept;
		bool operator !=(const Adapter &obj) const noexcept;
	};
}
