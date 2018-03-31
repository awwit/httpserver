#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ServerApplicationSettings.h"

namespace HttpServer
{
	class ServerApplicationsTree
	{
	protected:
		std::unordered_map<std::string, ServerApplicationsTree *> list;

		ServerApplicationSettings *app_sets;

	public:
		ServerApplicationsTree() noexcept;
		~ServerApplicationsTree() noexcept;

		bool empty() const noexcept;

		void addApplication(const std::string &name, ServerApplicationSettings *sets);
		void addApplication(std::vector<std::string> &nameParts, ServerApplicationSettings *sets);

		const ServerApplicationSettings *find(const std::string &name) const;
		const ServerApplicationSettings *find(std::vector<std::string> &nameParts) const;

		void collectApplicationSettings(std::unordered_set<ServerApplicationSettings *> &set) const;

		void clear() noexcept;
	};
}
