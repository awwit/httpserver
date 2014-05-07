#pragma once

#include "ServerRequest.h"
#include "ServerResponse.h"
#include "ResourceAbstract.h"

#include <string>

namespace HttpServer
{
	class ResourceMethodAbstract
	{
	protected:
		std::string resource_method_name;

	public:
		inline std::string getName() const
		{
			return resource_method_name;
		}

		virtual void execute(ResourceAbstract *, ServerRequest &, ServerResponse &) = 0;
	};
};
