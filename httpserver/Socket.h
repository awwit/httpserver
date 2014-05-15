#pragma once

#ifdef WIN32
	#include <WinSock2.h>
	#pragma comment(lib, "ws2_32.lib")
	#undef max
#elif POSIX
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <unistd.h>
	#include <fcntl.h>
#else
	#error "Undefine platform"
#endif

#include "System.h"

#include <cstddef>
#include <cstdint>

#include <vector>
#include <string>
#include <chrono>

namespace HttpServer
{
	class Socket
	{
	protected:
		System::native_socket_type socket_handle;

	public:
		int static Startup();
		int static Cleanup();

	public:
		Socket();
		Socket(const System::native_socket_type);
		~Socket() = default;

		System::native_socket_type open();
		int close();

		inline bool is_open() const
		{
			return -1 != socket_handle;
		}

		int bind(const int) const;
		int listen() const;
		Socket accept() const;
		int shutdown() const;

		bool nonblock(bool = true);

		size_t recv(std::vector<std::string::value_type> &) const;
		size_t nonblock_recv(std::vector<std::string::value_type> &, const std::chrono::milliseconds &) const;

		size_t send(const std::string &) const;
		size_t send(const std::vector<std::string::value_type> &, const size_t) const;

		size_t nonblock_send(const std::string &, const std::chrono::milliseconds &) const;
		size_t nonblock_send(const std::vector<std::string::value_type> &, const size_t, const std::chrono::milliseconds &) const;

		inline System::native_socket_type get_handle() const
		{
			return socket_handle;
		}

		Socket &operator =(const Socket);
	};
};
