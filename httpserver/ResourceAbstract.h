#pragma once

#include <unordered_map>

namespace HttpServer
{
	class ResourceMethodAbstract;

	class ResourceAbstract
	{
	protected:
		std::string resource_name;

	public:
		std::unordered_map<std::string, ResourceMethodAbstract *> methods;

		std::unordered_map<std::string, ResourceAbstract *> resources;

	private:
		void addMethod(ResourceMethodAbstract *);
		void addResource(ResourceAbstract *);

	public:
		ResourceAbstract();
		~ResourceAbstract();

		inline std::string getName() const
		{
			return resource_name;
		}
	};
};
