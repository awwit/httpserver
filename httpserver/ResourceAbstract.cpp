#include "ResourceAbstract.h"

#include "ResourceMethodAbstract.h"
#include "ResourceMethodOptions.h"

namespace HttpServer
{
	ResourceAbstract::ResourceAbstract()
	{
		addMethod(new ResourceMethodOptions() );
	}

	ResourceAbstract::~ResourceAbstract()
	{
		for (auto &method : methods)
		{
			delete method.second;
		}

		for (auto &resource : resources)
		{
			delete resource.second;
		}
	}

	void ResourceAbstract::addMethod(ResourceMethodAbstract *method)
	{
		methods.emplace(method->getName(), method);
	}

	void ResourceAbstract::addResource(ResourceAbstract *resource)
	{
		resources.emplace(resource->getName(), resource);
	}
};
