#pragma once

#include "Socket.h"

namespace HttpServer
{
	class SocketList
	{
	protected:
	#ifdef WIN32
		HANDLE obj_list;
		mutable std::vector<WSAPOLLFD> poll_events;
	#elif POSIX
		int obj_list;
		mutable std::vector<struct ::epoll_event> epoll_events;
	#else
		#error "Undefine platform"
	#endif

	public:
		SocketList();

		bool create(const size_t startListSize = 1);
		void destroy();

		bool is_created() const;

		bool addSocket(const Socket &sock);
		bool removeSocket(const Socket &sock);

		bool accept(std::vector<Socket> &sockets) const;

		bool recv(std::vector<Socket> &sockets, std::vector<Socket> &errors, std::chrono::milliseconds timeout = std::chrono::milliseconds(~0) ) const;
	};
};