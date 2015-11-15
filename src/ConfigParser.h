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
		bool includeConfigFile(const std::string &fileName, std::string &strBuf, const size_t offset = 0);

		bool addApplication(
			const std::unordered_map<std::string, std::string> &app,
			const ServerApplicationDefaultSettings &defaults,
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