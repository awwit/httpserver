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
		size_t obj_list;
		mutable std::vector<struct ::epoll_event> epoll_events;
	#else
		#error "Undefine platform"
	#endif

	public:
		SocketList();

		bool create(const size_t = 0);
		void destroy();

		inline bool is_created() const
		{
		#ifdef WIN32
			return INVALID_HANDLE_VALUE != this->obj_list;
		#elif POSIX
			return std::numeric_limits<size_t>::max() != this->obj_list;
		#else
			#error "Undefine platform"
		#endif
		}

		bool addSocket(const Socket &);
		bool removeSocket(const Socket &);

		bool accept(std::vector<Socket> &sockets) const;

		bool recv(std::vector<Socket> &sockets, std::vector<Socket> &errors) const;
	};
};