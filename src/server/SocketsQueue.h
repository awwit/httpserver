#pragma once

#include "../socket/Socket.h"
#include "../system/Cache.h"

#include "../transfer/http2/Http2.h"

#include <mutex>
#include <queue>

namespace HttpServer
{
	class SocketsQueue :
		public std::queue<std::tuple<Socket::Socket, Http2::IncStream *> >,
		System::CachePadding<std::queue<std::tuple<Socket::Socket, Http2::IncStream *> > >,
		public std::mutex
	{

	};
}
