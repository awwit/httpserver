
#include "SocketAdapter.h"

namespace HttpServer
{
	bool SocketAdapter::operator ==(const SocketAdapter &obj) const
	{
		return this->get_handle() == obj.get_handle();
	}

	bool SocketAdapter::operator !=(const SocketAdapter &obj) const
	{
		return this->get_handle() != obj.get_handle();
	}
};