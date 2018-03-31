#pragma once

#include "../SocketsQueue.h"

#include "ServerHttp2Protocol.h"
#include "../../transfer/HttpStatusCode.h"

namespace HttpServer
{
	class ServerHttp2 : public ServerHttp2Protocol
	{
	protected:
		SocketsQueue &sockets;

	public:
		ServerHttp2(
			Socket::Adapter &sock,
			const ServerSettings &settings,
			ServerControls &controls,
			SocketsQueue &sockets
		) noexcept;

		virtual ServerProtocol *process() override;
		virtual void close() override;
	};
}
