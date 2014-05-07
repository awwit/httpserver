#pragma once

#include <stack>
#include <ostream>

namespace HttpServer
{
	class ServerOutput
	{
	protected:
		std::stack<std::ostream *> streams;
	};
};
