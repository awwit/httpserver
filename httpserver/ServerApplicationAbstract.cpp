#include "ServerApplicationAbstract.h"

namespace HttpServer
{
	ServerApplicationAbstract::~ServerApplicationAbstract()
	{
		for (auto &resource : resources)
		{
			delete resource.second;
		}
	}

	void ServerApplicationAbstract::addResource(ResourceAbstract *resource)
	{
		resources.emplace(resource->getName(), resource);
	}
};
