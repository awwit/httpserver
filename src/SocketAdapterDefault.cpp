
#include "SocketAdapterDefault.h"

namespace HttpServer
{
	SocketAdapterDefault::SocketAdapterDefault(const Socket &_sock) : sock(_sock)
	{

	}

	System::native_socket_type SocketAdapterDefault::get_handle() const
	{
		return sock.get_handle();
	}

	::gnutls_session_t SocketAdapterDefault::get_tls_session() const
	{
		return 0;
	}

	SocketAdapter *SocketAdapterDefault::copy() const
	{
		return new SocketAdapterDefault(this->sock);
	}

	long SocketAdapterDefault::nonblock_recv(std::vector<std::string::value_type> &buf, const std::chrono::milliseconds &timeout) const
	{
		return sock.nonblock_recv(buf, timeout);
	}

	long SocketAdapterDefault::nonblock_send(const std::string &buf, const std::chrono::milliseconds &timeout) const
	{
		return sock.nonblock_send(buf, timeout);
	}

	long SocketAdapterDefault::nonblock_send(const std::vector<std::string::value_type> &buf, const size_t length, const std::chrono::milliseconds &timeout) const
	{
		return sock.nonblock_send(buf, length, timeout);
	}

	void SocketAdapterDefault::close()
	{
		// Wait for send all data to client
		sock.nonblock_send_sync();

		sock.shutdown();
		sock.close();
	}
};