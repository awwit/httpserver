
#include "SocketList.h"

namespace HttpServer
{
	SocketList::SocketList()
	{
	#ifdef WIN32
		obj_list = INVALID_HANDLE_VALUE;
	#elif POSIX
		obj_list = ~0;
	#else
		#error "Undefine platform"
	#endif
	}

	bool SocketList::create(const size_t listSize)
	{
		destroy();

	#ifdef WIN32
		obj_list = (HANDLE) 1;

		if (listSize > 0)
		{
			poll_events.reserve(listSize);
		}

		return true;
	#elif POSIX
		obj_list = ::epoll_create(listSize);

		if ( (size_t)~0 == obj_list)
		{
			return false;
		}

		if (listSize > 0)
		{
			epoll_events.reserve(listSize);
		}

		return true;
	#else
		#error "Undefine platform"
	#endif
	}

	void SocketList::destroy()
	{
		if (is_created() )
		{
		#ifdef WIN32
			obj_list = INVALID_HANDLE_VALUE;
			poll_events.clear();
		#elif POSIX
			::close(obj_list);
			obj_list = ~0;
			epoll_events.clear();
		#else
			#error "Undefine platform"
		#endif
		}
	}

	bool SocketList::addSocket(const Socket &sock)
	{
		if (is_created() )
		{
		#ifdef WIN32
            WSAPOLLFD event = {
                sock.get_handle(),
                POLLRDNORM,
                0
            };

			poll_events.emplace_back(event);

			return true;
		#elif POSIX
            struct ::epoll_event event = {
                EPOLLIN | EPOLLET,
                reinterpret_cast<void *>(sock.get_handle() )
            };

			size_t result = ::epoll_ctl(obj_list, EPOLL_CTL_ADD, sock.get_handle(), &event);

			if ( (size_t)~0 == result)
			{
				return false;
			}

			epoll_events.emplace_back();

			return true;
		#else
			#error "Undefine platform"
		#endif
		}

		return false;
	}

	bool SocketList::accept(std::vector<Socket> &sockets) const
	{
		if (is_created() )
		{
		#ifdef WIN32
			size_t count = ::WSAPoll(poll_events.data(), poll_events.size(), ~0);

			if (SOCKET_ERROR == count)
			{
				return false;
			}

			for (size_t i = 0; i < poll_events.size(); ++i)
			{
				WSAPOLLFD &event = poll_events[i];

				if (event.revents | POLLRDNORM)
				{
					System::native_socket_type client_socket = ~0;

					do
					{
						client_socket = ::accept(event.fd, static_cast<sockaddr *>(nullptr), static_cast<int *>(nullptr) );

						if (~0 != client_socket)
						{
							sockets.emplace_back(Socket(client_socket) );
						}
					}
					while (~0 != client_socket);
				}
			}

			return false == sockets.empty();
		#elif POSIX
			size_t count = ::epoll_wait(obj_list, epoll_events.data(), epoll_events.size(), ~0);

            if (std::numeric_limits<size_t>::max() == count)
			{
				return false;
			}

			for (size_t i = 0; i < count; ++i)
			{
				epoll_event &event = epoll_events[i];

				if (event.events | EPOLLIN)
				{
					System::native_socket_type client_socket = ~0;

					do
					{
						client_socket = ::accept(event.data.fd, static_cast<sockaddr *>(nullptr), static_cast<socklen_t *>(nullptr) );

						if (~0 != client_socket)
						{
							sockets.emplace_back(Socket(client_socket) );
						}
					}
					while (~0 != client_socket);
				}
			}

			return false == sockets.empty();
		#else
			#error "Undefine platform"
		#endif
		}

		return false;
	}
};