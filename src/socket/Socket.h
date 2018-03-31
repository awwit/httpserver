#pragma once

#include "../system/System.h"

#include <vector>
#include <string>
#include <chrono>

namespace Socket
{
	class Socket
	{
	protected:
		System::native_socket_type socket_handle;

	public:
		bool static Startup() noexcept;
		bool static Cleanup() noexcept;
		int static getLastError() noexcept;

	public:
		Socket() noexcept;
		Socket(const System::native_socket_type fd) noexcept;
		Socket(const Socket &obj) noexcept;
		Socket(Socket &&obj) noexcept;

		~Socket() noexcept = default;

		bool open() noexcept;
		bool close() noexcept;

		bool is_open() const noexcept;
		System::native_socket_type get_handle() const noexcept;

		bool bind(const int port) const noexcept;
		bool listen() const noexcept;

		Socket accept() const noexcept;
		Socket nonblock_accept() const noexcept;

		Socket nonblock_accept(
			const std::chrono::milliseconds &timeout
		) const noexcept;

		bool shutdown() const noexcept;

		bool nonblock(const bool isNonBlock = true) const noexcept;
	//	bool is_nonblock() const noexcept;
		bool tcp_nodelay(const bool nodelay = true) const noexcept;

		long recv(
			std::vector<std::string::value_type> &buf
		) const noexcept;

		long recv(
			void *buf,
			const size_t length
		) const noexcept;

		long nonblock_recv(
			std::vector<std::string::value_type> &buf,
			const std::chrono::milliseconds &timeout
		) const noexcept;

		long nonblock_recv(
			void *buf,
			const size_t length,
			const std::chrono::milliseconds &timeout
		) const noexcept;

		bool nonblock_recv_sync(
			const std::chrono::milliseconds &timeout
		) const noexcept;

		long send(const std::string &buf) const noexcept;
		long send(const void *buf, const size_t length) const noexcept;

		long nonblock_send(
			const std::string &buf,
			const std::chrono::milliseconds &timeout
		) const noexcept;

		long nonblock_send(
			const void *buf,
			const size_t length,
			const std::chrono::milliseconds &timeout
		) const noexcept;

		void nonblock_send_sync() const noexcept;

		Socket &operator =(const Socket &obj) noexcept;

		bool operator ==(const Socket &obj) const noexcept;
		bool operator !=(const Socket &obj) const noexcept;
	};
}

namespace std
{
	// Hash for Socket
	template<> struct hash<Socket::Socket> {
		std::size_t operator()(const Socket::Socket &obj) const noexcept {
			return std::hash<System::native_socket_type>{}(obj.get_handle() );
		}
	};
}
