#pragma once

#ifdef WIN32
	#include <WinSock2.h>
	#pragma comment(lib, "ws2_32.lib")
	#undef max
#elif POSIX
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/epoll.h>
	#include <poll.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
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
		bool static Startup();
		bool static Cleanup();
		int static getLastError();

	public:
		Socket();
		Socket(const System::native_socket_type fd);
		Socket(const Socket &obj);
		Socket(Socket &&obj);

		~Socket() = default;

		bool open();
		bool close();

		inline bool is_open() const
		{
		#ifdef WIN32
			return INVALID_SOCKET != this->socket_handle;
		#elif POSIX
			return ~0 != this->socket_handle;
		#else
			#error "Undefine platform"
		#endif
		}

		bool bind(const int port) const;
		bool listen() const;

		Socket accept() const;
		Socket nonblock_accept() const;
		Socket nonblock_accept(const std::chrono::milliseconds &timeout) const;

		bool shutdown() const;

		bool nonblock(const bool isNonBlock = true) const;
	//	bool is_nonblock() const;
		bool tcp_nodelay(const bool nodelay = true) const;

		long recv(std::vector<std::string::value_type> &buf) const;
		long nonblock_recv(std::vector<std::string::value_type> &buf, const std::chrono::milliseconds &timeout) const;

		long send(const std::string &buf) const;
		long send(const std::vector<std::string::value_type> &buf, const size_t length) const;

		long nonblock_send(const std::string &buf, const std::chrono::milliseconds &timeout) const;
		long nonblock_send(const std::vector<std::string::value_type> &buf, const size_t length, const std::chrono::milliseconds &timeout) const;

		void nonblock_send_sync() const;

		inline System::native_socket_type get_handle() const
		{
			return this->socket_handle;
		}

		Socket &operator =(const Socket &obj);

		bool operator ==(const Socket &obj) const;
		bool operator !=(const Socket &obj) const;
	};
};

namespace std
{
	// Hash for Socket
	template<> struct hash<HttpServer::Socket>
	{
		std::size_t operator()(const HttpServer::Socket &obj) const
		{
			return std::hash<System::native_socket_type>{}(obj.get_handle() );
		}
	};
};