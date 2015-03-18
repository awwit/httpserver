
#include "Socket.h"

namespace HttpServer
{
	bool Socket::Startup()
	{
	#ifdef WIN32
		unsigned short version = MAKEWORD(2, 2);
		::WSADATA wsaData = {0};
		return 0 == ::WSAStartup(version, &wsaData);
	#elif POSIX
		return true;
	#else
		#error "Undefine platform"
	#endif
	}

	bool Socket::Cleanup()
	{
	#ifdef WIN32
		return 0 == ::WSACleanup();
	#elif POSIX
		return true;
	#else
		#error "Undefine platform"
	#endif
	}

	Socket::Socket(): socket_handle(~0)
	{
		
	}

	Socket::Socket(const System::native_socket_type handle) : socket_handle(handle)
	{
		
	}

	Socket::Socket(const Socket &obj) : socket_handle(obj.socket_handle)
	{
		
	}

	Socket::Socket(Socket &&that) : socket_handle(that.socket_handle)
	{
		that.socket_handle = ~0;
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
				socket_handle = ~0;
			}

			return result;
		}

		return ~0;
	}

	int Socket::bind(const int port) const
	{
        ::sockaddr_in sock_addr = {
            AF_INET,
            htons(port),
            ::htonl(INADDR_ANY),
            0
        };

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

	Socket Socket::nonblock_accept() const
	{
		System::native_socket_type client_socket = ~0;
	#ifdef WIN32
        WSAPOLLFD event = {
            socket_handle,
            POLLRDNORM,
            0
        };

        if (1 == ::WSAPoll(&event, 1, ~0) && event.revents & POLLRDNORM)
		{
			client_socket = ::accept(socket_handle, static_cast<sockaddr *>(nullptr), static_cast<int *>(nullptr) );
		}
	#elif POSIX
        struct ::pollfd event = {
            socket_handle,
            POLLIN,
            0
        };

        if (1 == ::poll(&event, 1, ~0) && event.revents & POLLIN)
		{
			client_socket = ::accept(socket_handle, static_cast<sockaddr *>(nullptr), static_cast<socklen_t *>(nullptr) );
		}
	#else
		#error "Undefine platform"
	#endif
		return Socket(client_socket);
	}

	Socket Socket::nonblock_accept(const std::chrono::milliseconds &timeWait) const
	{
		System::native_socket_type client_socket = ~0;
	#ifdef WIN32
        WSAPOLLFD event = {
            socket_handle,
            POLLRDNORM,
            0
        };

        if (1 == ::WSAPoll(&event, 1, timeWait.count() ) && event.revents & POLLRDNORM)
		{
			client_socket = ::accept(socket_handle, static_cast<sockaddr *>(nullptr), static_cast<int *>(nullptr) );
		}
	#elif POSIX
        struct ::pollfd event = {
            socket_handle,
            POLLIN,
            0
        };

        if (1 == ::poll(&event, 1, timeWait.count() ) && event.revents & POLLIN)
		{
			client_socket = ::accept(socket_handle, static_cast<sockaddr *>(nullptr), static_cast<socklen_t *>(nullptr) );
		}
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

	bool Socket::nonblock(const bool isNonBlock) const
	{
	#ifdef WIN32
		unsigned long value = isNonBlock;
		return 0 == ::ioctlsocket(socket_handle, FIONBIO, &value);
	#elif POSIX
		return -1 != ::fcntl(socket_handle, F_SETFL, isNonBlock ? O_NONBLOCK : O_SYNC);
	#else
		#error "Undefine platform"
	#endif
	}

/*	bool Socket::is_nonblock() const
	{
	#ifdef WIN32
		
	#elif POSIX
		int flags = ::fcntl(socket_handle, F_GETFL, 0);
		return (flags != -1) && (flags & O_NONBLOCK);
	#else
		#error "Undefine platform"
	#endif
	}*/

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

	size_t Socket::nonblock_recv(std::vector<std::string::value_type> &buf, const std::chrono::milliseconds &timeWait) const
	{
		size_t recv_len = ~0;
	#ifdef WIN32
        WSAPOLLFD event = {
            socket_handle,
            POLLRDNORM,
            0
        };

        if (1 == ::WSAPoll(&event, 1, timeWait.count() ) && event.revents & POLLRDNORM)
        {
			recv_len = ::recv(socket_handle, buf.data(), buf.size(), 0);
		}
	#elif POSIX
        struct ::pollfd event = {
            socket_handle,
            POLLIN,
            0
        };

        if (1 == ::poll(&event, 1, timeWait.count() ) && event.revents & POLLIN)
		{
			recv_len = ::recv(socket_handle, buf.data(), buf.size(), MSG_NOSIGNAL);
		}
	#else
		#error "Undefine platform"
	#endif
		return recv_len;
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

	size_t Socket::nonblock_send(const std::string &buf, const std::chrono::milliseconds &timeWait) const
	{
		size_t send_len = ~0;
	#ifdef WIN32
        WSAPOLLFD event = {
            socket_handle,
            POLLWRNORM,
            0
        };

        if (1 == ::WSAPoll(&event, 1, timeWait.count() ) && event.revents & POLLWRNORM)
		{
			send_len = ::send(socket_handle, buf.data(), buf.length(), 0);
		}
	#elif POSIX
        struct ::pollfd event = {
            socket_handle,
            POLLOUT,
            0
        };

        if (1 == ::poll(&event, 1, timeWait.count() ) && event.revents & POLLOUT)
		{
			send_len = ::send(socket_handle, buf.data(), buf.length(), MSG_NOSIGNAL);
		}
	#else
		#error "Undefine platform"
	#endif
		return send_len;
	}

	size_t Socket::nonblock_send(const std::vector<std::string::value_type> &buf, const size_t length, const std::chrono::milliseconds &timeWait) const
	{
		size_t send_len = ~0;
	#ifdef WIN32
        WSAPOLLFD event = {
            socket_handle,
            POLLWRNORM,
            0
        };

        if (1 == ::WSAPoll(&event, 1, timeWait.count() ) && event.revents & POLLWRNORM)
		{
			send_len = ::send(socket_handle, buf.data(), length, 0);
		}
	#elif POSIX
        struct ::pollfd event = {
            socket_handle,
            POLLOUT,
            0
        };

        if (1 == ::poll(&event, 1, timeWait.count() ) && event.revents & POLLOUT)
		{
			send_len = ::send(socket_handle, buf.data(), length, MSG_WAITALL | MSG_NOSIGNAL);
		}
	#else
		#error "Undefine platform"
	#endif
		return send_len;
	}

	Socket &Socket::operator=(const Socket s)
	{
		socket_handle = s.socket_handle;
		return *this;
	}
};
