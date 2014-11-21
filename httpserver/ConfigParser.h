#pragma once

#include "ServerApplicationDefaultSettings.h"
#include "ServerApplicationsTree.h"
#include "Module.h"

#include <unordered_map>
#include <vector>

namespace HttpServer
{
	class ConfigParser
	{
	private:
		bool includeConfigFile(const std::string &, std::string &, const size_t);

		bool addApplication(
			const std::unordered_map<std::string, std::string> &,
			const ServerApplicationDefaultSettings &,
			std::vector<Module> &modules,
			ServerApplicationsTree &apps_tree
		);

		bool parseMimes(const std::string &fileName, std::unordered_map<std::string, std::string> &mimes_types);

	public:
		bool loadConfig(
			const std::string &conf,
			std::unordered_map<std::string, std::string> &settings,
			std::unordered_map<std::string, std::string> &mimes_types,
			std::vector<Module> &modules,
			ServerApplicationsTree &apps_tree
		);
	};
};