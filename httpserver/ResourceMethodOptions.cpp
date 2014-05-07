#include "ResourceMethodOptions.h"

namespace HttpServer
{
	ResourceMethodOptions::ResourceMethodOptions()
	{
		resource_method_name = "OPTIONS";
	}

	void ResourceMethodOptions::execute(ResourceAbstract *resource, ServerRequest &request, ServerResponse &response)
	{
		std::string options;

		for (auto &method : resource->methods)
		{
			if (method.second != this)
			{
				options += method.first;
				options.push_back(',');
			}
		}

		if (options.length() )
		{
			options.pop_back();
		}
	}
};
