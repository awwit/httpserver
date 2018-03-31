#pragma once

#include "Socket.h"

#ifdef POSIX
	#include <sys/epoll.h>
	#include <netinet/in.h>
#endif

namespace Socket
{
	class List
	{
	protected:
	#ifdef WIN32
		HANDLE obj_list;
		mutable std::vector<WSAPOLLFD> poll_events;
	#elif POSIX
		int obj_list;
		mutable std::vector<struct ::epoll_event> epoll_events;
	#else
		#error "Undefined platform"
	#endif

	public:
		List() noexcept;
		List(List &&obj) noexcept;
		~List() noexcept;

		bool create(const size_t startListSize = 1);
		void destroy() noexcept;

		bool is_created() const noexcept;

		bool addSocket(const Socket &sock) noexcept;
		bool removeSocket(const Socket &sock) noexcept;

		bool accept(
			std::vector<Socket> &sockets
		) const noexcept;

		bool accept(
			std::vector<Socket> &sockets,
			std::vector<struct sockaddr_in> &socketsAddress
		) const noexcept;

		bool recv(
			std::vector<Socket> &sockets,
			std::vector<Socket> &errors,
			std::chrono::milliseconds timeout = std::chrono::milliseconds(~0)
		) const noexcept;
	};
}
