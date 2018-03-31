#pragma once

#include "data-variant/Abstract.h"

#include "ServerApplicationsTree.h"

namespace HttpServer
{
	class ServerSettings
	{
	public:
		std::unordered_map<std::string, std::string> global;
		std::unordered_map<std::string, DataVariant::Abstract *> variants;
		std::unordered_map<std::string, std::string> mimes_types;
		ServerApplicationsTree apps_tree;

	public:
		ServerSettings() = default;
		~ServerSettings();

		void addDataVariant(DataVariant::Abstract *dataVariant);

		void clear();
	};
}
