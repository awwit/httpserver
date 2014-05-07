
#include "Socket.h"

namespace HttpServer
{
	int Socket::Startup()
	{
	#ifdef WIN32
		unsigned short version = MAKEWORD(2, 2);
		WSADATA wsaData = {0};
		return WSAStartup(version, &wsaData);
	#elif POSIX
		return 0;
	#else
		#error "Undefine platform"
	#endif
	}

	int Socket::Cleanup()
	{
	#ifdef WIN32
		return WSACleanup();
	#elif POSIX
		return 0;
	#else
		#error "Undefine platform"
	#endif
	}

	Socket::Socket(): socket_handle(-1)
	{
		
	}

	Socket::Socket(const System::native_socket_type handle)
	{
		socket_handle = handle;
	}

	System::native_socket_type Socket::open()
	{
		close();

		socket_handle = ::socket(AF_INET, SOCK_STREAM, 0);

		return socket_handle;
	}

	int Socket::close()
	{
		if (is_open() )
		{
		#ifdef WIN32
			int result = ::closesocket(socket_handle);
		#elif POSIX
			int result = ::close(socket_handle);
		#else
			#error "Undefine platform"
		#endif

			if (0 == result)
			{
				socket_handle = -1;
			}

			return result;
		}

		return -1;
	}

	int Socket::bind(const int port) const
	{
		sockaddr_in sock_addr = {0};
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_port = htons(port);
		sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);

		return ::bind(socket_handle, reinterpret_cast<sockaddr *>(&sock_addr), sizeof(sockaddr_in) );
	}

	int Socket::listen() const
	{
		return ::listen(socket_handle, SOMAXCONN);
	}

	Socket Socket::accept() const
	{
	#ifdef WIN32
		System::native_socket_type client_socket = ::accept(socket_handle, static_cast<sockaddr *>(nullptr), static_cast<int *>(nullptr) );
	#elif POSIX
		System::native_socket_type client_socket = ::accept(socket_handle, static_cast<sockaddr *>(nullptr), static_cast<socklen_t *>(nullptr) );
	#else
		#error "Undefine platform"
	#endif

		return Socket(client_socket);
	}

	int Socket::shutdown() const
	{
		if (is_open() )
		{
		#ifdef WIN32
			return ::shutdown(socket_handle, SD_BOTH);
		#elif POSIX
			return ::shutdown(socket_handle, SHUT_RDWR);
		#else
			#error "Undefine platform"
		#endif
		}

		return -1;
	}

	size_t Socket::recv(std::vector<std::string::value_type> &buf) const
	{
	#ifdef WIN32
		return ::recv(socket_handle, buf.data(), buf.size(), 0);
	#elif POSIX
		return ::recv(socket_handle, buf.data(), buf.size(), MSG_NOSIGNAL);
	#else
		#error "Undefine platform"
	#endif
	}

	size_t Socket::send(const std::string &buf) const
	{
	#ifdef WIN32
		return ::send(socket_handle, buf.data(), buf.length(), 0);
	#elif POSIX
		return ::send(socket_handle, buf.data(), buf.length(), MSG_WAITALL | MSG_NOSIGNAL);
	#else
		#error "Undefine platform"
	#endif
	}

	size_t Socket::send(const std::vector<std::string::value_type> &buf, const size_t length) const
	{
	#ifdef WIN32
		return ::send(socket_handle, buf.data(), length, 0);
	#elif POSIX
		return ::send(socket_handle, buf.data(), length, MSG_WAITALL | MSG_NOSIGNAL);
	#else
		#error "Undefine platform"
	#endif
	}

	Socket &Socket::operator=(const Socket s)
	{
		socket_handle = s.socket_handle;
		return *this;
	}
};
