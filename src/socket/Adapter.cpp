
#include "Adapter.h"

namespace Socket
{
	long Adapter::nonblock_recv(
		std::vector<std::string::value_type> &buf,
		const std::chrono::milliseconds &timeout
	) const noexcept {
		return this->nonblock_recv(
			buf.data(),
			buf.size(),
			timeout
		);
	}

	long Adapter::nonblock_send(
		const std::string &buf,
		const std::chrono::milliseconds &timeout
	) const noexcept {
		return this->nonblock_send(
			buf.data(),
			buf.length(),
			timeout
		);
	}

	bool Adapter::operator ==(const Adapter &obj) const noexcept {
		return this->get_handle() == obj.get_handle();
	}

	bool Adapter::operator !=(const Adapter &obj) const noexcept {
		return this->get_handle() != obj.get_handle();
	}
}
