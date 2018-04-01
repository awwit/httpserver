
#include "Socket.h"

#ifdef POSIX
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <poll.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <unistd.h>
	#include <fcntl.h>
#endif

namespace Socket
{
	bool Socket::Startup() noexcept
	{
	#ifdef WIN32
		unsigned short version = MAKEWORD(2, 2);
		::WSADATA wsaData = {0};
		return 0 == ::WSAStartup(version, &wsaData);
	#elif POSIX
		return true;
	#else
		#error "Undefined platform"
	#endif
	}

	bool Socket::Cleanup() noexcept
	{
	#ifdef WIN32
		return 0 == ::WSACleanup();
	#elif POSIX
		return true;
	#else
		#error "Undefined platform"
	#endif
	}

	int Socket::getLastError() noexcept
	{
	#ifdef WIN32
		return ::WSAGetLastError();
	#elif POSIX
		return errno;
	#else
		#error "Undefined platform"
	#endif
	}

	Socket::Socket() noexcept : socket_handle(~0) {}

	Socket::Socket(const System::native_socket_type fd) noexcept : socket_handle(fd) {}

	Socket::Socket(const Socket &obj) noexcept : socket_handle(obj.socket_handle) {}

	Socket::Socket(Socket &&obj) noexcept
		: socket_handle(obj.socket_handle)
	{
		obj.socket_handle = ~0;
	}

	bool Socket::open() noexcept {
		this->close();
		this->socket_handle = ::socket(AF_INET, SOCK_STREAM, 0);
		return this->is_open();
	}

	bool Socket::close() noexcept
	{
		if (this->is_open() )
		{
		#ifdef WIN32
			const int result = ::closesocket(this->socket_handle);
		#elif POSIX
			const int result = ::close(this->socket_handle);
		#else
			#error "Undefined platform"
		#endif

			if (0 == result) {
				this->socket_handle = ~0;
				return true;
			}
		}

		return false;
	}

	bool Socket::is_open() const noexcept
	{
	#ifdef WIN32
		return INVALID_SOCKET != this->socket_handle;
	#elif POSIX
		return ~0 != this->socket_handle;
	#else
		#error "Undefined platform"
	#endif
	}

	System::native_socket_type Socket::get_handle() const noexcept {
		return this->socket_handle;
	}

	bool Socket::bind(const int port) const noexcept {
		const ::sockaddr_in sock_addr {
			AF_INET,
			::htons(port),
			::htonl(INADDR_ANY),
			0
		};

		return 0 == ::bind(
			this->socket_handle,
			reinterpret_cast<const sockaddr *>(&sock_addr),
			sizeof(sockaddr_in)
		);
	}

	bool Socket::listen() const noexcept {
		return 0 == ::listen(this->socket_handle, SOMAXCONN);
	}

	Socket Socket::accept() const noexcept
	{
	#ifdef WIN32
		System::native_socket_type client_socket = ::accept(
			this->socket_handle,
			static_cast<sockaddr *>(nullptr),
			static_cast<int *>(nullptr)
		);
	#elif POSIX
		System::native_socket_type client_socket = ::accept(
			this->socket_handle,
			static_cast<sockaddr *>(nullptr),
			static_cast<socklen_t *>(nullptr)
		);
	#else
		#error "Undefined platform"
	#endif
		return Socket(client_socket);
	}

	Socket Socket::nonblock_accept() const noexcept
	{
		System::native_socket_type client_socket = ~0;
	#ifdef WIN32
		WSAPOLLFD event {
			this->socket_handle,
            POLLRDNORM,
            0
        };

		if (1 == ::WSAPoll(&event, 1, ~0) && event.revents & POLLRDNORM) {
			client_socket = ::accept(
				this->socket_handle,
				static_cast<sockaddr *>(nullptr),
				static_cast<int *>(nullptr)
			);
		}
	#elif POSIX
		struct ::pollfd event {
			this->socket_handle,
            POLLIN,
            0
        };

		if (1 == ::poll(&event, 1, ~0) && event.revents & POLLIN) {
			client_socket = ::accept(
				this->socket_handle,
				static_cast<sockaddr *>(nullptr),
				static_cast<socklen_t *>(nullptr)
			);
		}
	#else
		#error "Undefined platform"
	#endif
		return Socket(client_socket);
	}

	Socket Socket::nonblock_accept(const std::chrono::milliseconds &timeout) const noexcept
	{
		System::native_socket_type client_socket = ~0;
	#ifdef WIN32
		WSAPOLLFD event {
			this->socket_handle,
			POLLRDNORM,
			0
		};

		if (1 == ::WSAPoll(&event, 1, int(timeout.count()) ) && event.revents & POLLRDNORM) {
			client_socket = ::accept(
				this->socket_handle,
				static_cast<sockaddr *>(nullptr),
				static_cast<int *>(nullptr)
			);
		}
	#elif POSIX
		struct ::pollfd event {
			this->socket_handle,
			POLLIN,
			0
		};

		if (1 == ::poll(&event, 1, int(timeout.count()) ) && event.revents & POLLIN) {
			client_socket = ::accept(
				this->socket_handle,
				static_cast<sockaddr *>(nullptr),
				static_cast<socklen_t *>(nullptr)
			);
		}
	#else
		#error "Undefined platform"
	#endif
		return Socket(client_socket);
	}

	bool Socket::shutdown() const noexcept
	{
		if (is_open() )
		{
		#ifdef WIN32
			return 0 == ::shutdown(this->socket_handle, SD_BOTH);
		#elif POSIX
			return 0 == ::shutdown(this->socket_handle, SHUT_RDWR);
		#else
			#error "Undefined platform"
		#endif
		}

		return false;
	}

	bool Socket::nonblock(const bool isNonBlock) const noexcept
	{
	#ifdef WIN32
		unsigned long value = isNonBlock;

		return 0 == ::ioctlsocket(
			this->socket_handle,
			FIONBIO,
			&value
		);
	#elif POSIX
		return ~0 != ::fcntl(
			this->socket_handle,
			F_SETFL,
			isNonBlock
				? O_NONBLOCK
				: O_SYNC
		);
	#else
		#error "Undefined platform"
	#endif
	}

/*
	bool Socket::is_nonblock() const noexcept
	{
	#ifdef WIN32
		
	#elif POSIX
		const int flags = ::fcntl(socket_handle, F_GETFL, 0);
		return (flags != ~0) && (flags & O_NONBLOCK);
	#else
		#error "Undefined platform"
	#endif
	}
*/

	bool Socket::tcp_nodelay(const bool nodelay) const noexcept
	{
	#ifdef WIN32
		int flags = nodelay ? 1 : 0;

		return 0 == ::setsockopt(
			this->socket_handle,
			IPPROTO_TCP,
			TCP_NODELAY,
			reinterpret_cast<char *>(&flags),
			sizeof(flags)
		);
	#elif POSIX
		int flags = nodelay ? 1 : 0;

		return 0 == ::setsockopt(
			this->socket_handle,
			IPPROTO_TCP,
			TCP_NODELAY,
			&flags,
			sizeof(flags)
		);
	#else
		#error "Undefined platform"
	#endif
	}

	long Socket::recv(
		std::vector<std::string::value_type> &buf
	) const noexcept {
		return this->recv(buf.data(), buf.size() );
	}

	long Socket::recv(
		void *buf,
		const size_t length
	) const noexcept {
	#ifdef WIN32
		return ::recv(
			this->socket_handle,
			reinterpret_cast<char *>(buf),
			static_cast<const int>(length),
			0
		);
	#elif POSIX
		return ::recv(this->socket_handle, buf, length, 0);
	#else
		#error "Undefined platform"
	#endif
	}

	long Socket::nonblock_recv(
		std::vector<std::string::value_type> &buf,
		const std::chrono::milliseconds &timeout
	) const noexcept {
		return this->nonblock_recv(
			buf.data(),
			buf.size(),
			timeout
		);
	}

	long Socket::nonblock_recv(
		void *buf,
		const size_t length,
		const std::chrono::milliseconds &timeout
	) const noexcept {
		long recv_len = ~0;
	#ifdef WIN32
		WSAPOLLFD event {
			this->socket_handle,
			POLLRDNORM,
			0
		};

		if (1 == ::WSAPoll(&event, 1, int(timeout.count()) ) && event.revents & POLLRDNORM) {
			recv_len = ::recv(
				this->socket_handle,
				reinterpret_cast<char *>(buf),
				static_cast<const int>(length),
				0
			);
		}
	#elif POSIX
		struct ::pollfd event {
			this->socket_handle,
			POLLIN,
			0
		};

		if (1 == ::poll(&event, 1, int(timeout.count()) ) && event.revents & POLLIN) {
			recv_len = ::recv(this->socket_handle, buf, length, 0);
		}
	#else
		#error "Undefined platform"
	#endif
		return recv_len;
	}

	bool Socket::nonblock_recv_sync(
		const std::chrono::milliseconds &timeout
	) const noexcept {
	#ifdef WIN32
		WSAPOLLFD event {
			this->socket_handle,
			POLLIN,
			0
		};

		return ::WSAPoll(&event, 1, int(timeout.count()) ) == 1;
	#elif POSIX
		struct ::pollfd event {
			this->socket_handle,
			POLLIN,
			0
		};

		return ::poll(&event, 1, int(timeout.count()) ) == 1;
	#else
		#error "Undefined platform"
	#endif
	}

	static long send_all(
		const System::native_socket_type socket_handle,
		const void *data,
		const size_t length
	) noexcept {
		size_t total = 0;

		while (total < length) {
			const long send_size = ::send(
				socket_handle,
				reinterpret_cast<const char *>(data) + total,
				length - total,
				0
			);

			if (send_size < 0) {
				return send_size;
			}

			total += size_t(send_size);
		}

		return static_cast<long>(total);
	}

	long Socket::send(const std::string &buf) const noexcept {
		return this->send(buf.data(), buf.length() );
	}

	long Socket::send(const void *buf, const size_t length) const noexcept {
		return send_all(this->socket_handle, buf, length);
	}

	static long nonblock_send_all(
		const System::native_socket_type socket_handle,
		const void *data,
		const size_t length,
		const std::chrono::milliseconds &timeout
	) noexcept {
		size_t total = 0;

	#ifdef WIN32
		WSAPOLLFD event = {
			socket_handle,
			POLLOUT,
			0
		};

		while (total < length) {
			if (1 == ::WSAPoll(&event, 1, int(timeout.count()) ) && event.revents & POLLOUT) {
				const long send_size = ::send(
					socket_handle,
					reinterpret_cast<const char *>(data) + total,
					static_cast<const int>(length - total),
					0
				);

				if (send_size < 0) {
					return send_size;
				}

				total += size_t(send_size);
			} else {
				return -1;
			}
		}

	#elif POSIX
		struct ::pollfd event = {
			socket_handle,
			POLLOUT,
			0
		};

		while (total < length) {
			if (1 == ::poll(&event, 1, int(timeout.count()) ) && event.revents & POLLOUT) {
				const long send_size = ::send(
					socket_handle,
					reinterpret_cast<const uint8_t *>(data) + total,
					length - total,
					0
				);

				if (send_size < 0) {
					return send_size;
				}

				total += size_t(send_size);
			} else {
				return -1;
			}
		}
	#else
		#error "Undefined platform"
	#endif

		return long(total);
	}

	long Socket::nonblock_send(
		const std::string &buf,
		const std::chrono::milliseconds &timeout
	) const noexcept {
		return this->nonblock_send(
			buf.data(),
			buf.length(),
			timeout
		);
	}

	long Socket::nonblock_send(
		const void *buf,
		const size_t length,
		const std::chrono::milliseconds &timeout
	) const noexcept {
		return nonblock_send_all(
			this->socket_handle,
			buf,
			length,
			timeout
		);
	}

	void Socket::nonblock_send_sync() const noexcept
	{
	#ifdef WIN32
		WSAPOLLFD event {
			this->socket_handle,
			POLLOUT,
			0
		};

		::WSAPoll(&event, 1, ~0);
	#elif POSIX
		struct ::pollfd event {
			this->socket_handle,
			POLLOUT,
			0
		};

		::poll(&event, 1, ~0);
	#else
		#error "Undefined platform"
	#endif
	}

	Socket &Socket::operator=(const Socket &obj) noexcept {
		this->socket_handle = obj.socket_handle;
		return *this;
	}

	bool Socket::operator ==(const Socket &obj) const noexcept {
		return this->socket_handle == obj.socket_handle;
	}

	bool Socket::operator !=(const Socket &obj) const noexcept {
		return this->socket_handle != obj.socket_handle;
	}
}
