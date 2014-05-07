#pragma once

#include "ResourceMethodAbstract.h"

namespace HttpServer
{
	class ResourceMethodOptions: public ResourceMethodAbstract
	{
	public:
		ResourceMethodOptions();

		virtual void execute(ResourceAbstract *, ServerRequest &, ServerResponse &) override;
	};
};
