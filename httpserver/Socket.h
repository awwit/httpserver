#pragma once

#ifdef WIN32
	#include <WinSock2.h>
	#pragma comment(lib, "ws2_32.lib")
#elif POSIX
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <unistd.h>
#else
	#error "Undefine platform"
#endif

#include "System.h"

#include <cstddef>
#include <cstdint>

#include <vector>
#include <string>

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
		size_t recv(std::vector<std::string::value_type> &) const;
		size_t send(const std::string &) const;
		size_t send(const std::vector<std::string::value_type> &, const size_t) const;

		inline System::native_socket_type get_handle() const
		{
			return socket_handle;
		}

		Socket &operator =(const Socket);
	};
};
