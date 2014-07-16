
#include "ServerApplicationsTree.h"

namespace HttpServer
{
	ServerApplicationsTree::ServerApplicationsTree(): app_sets(nullptr)
	{
		
	}

	ServerApplicationsTree::~ServerApplicationsTree()
	{
		clear();
	}

	void ServerApplicationsTree::addApplication(std::vector<std::string> &nameParts, ServerApplicationSettings *sets)
	{
		if (nameParts.empty() )
		{
			app_sets = sets;
		}
		else
		{
			std::string &part = nameParts.back();

			auto it = list.find(part);

			ServerApplicationsTree *sub;

			if (list.end() != it)
			{
				sub = it->second;
			}
			else
			{
				sub = new ServerApplicationsTree();
				list.emplace(std::move(part), sub);
			}

			nameParts.pop_back();

			sub->addApplication(nameParts, sets);
		}
	}

	void ServerApplicationsTree::addApplication(const std::string &name, ServerApplicationSettings *sets)
	{
		std::vector<std::string> name_parts;

		size_t delimiter = name.find('.');

		if (std::string::npos != delimiter)
		{
			size_t cur_pos = 0;

			while (std::string::npos != delimiter)
			{
				std::string part = name.substr(cur_pos, delimiter - cur_pos);

				if ("" == part)
				{
					part = "*";
				}

				name_parts.emplace_back(std::move(part) );
				cur_pos = delimiter + 1;
				delimiter = name.find('.', cur_pos);
			}

			// Emplace last part
			std::string part = name.substr(cur_pos);
			name_parts.emplace_back(std::move(part) );
		}
		else
		{
			name_parts.emplace_back(name);
		}

		addApplication(name_parts, sets);
	}

	ServerApplicationSettings *ServerApplicationsTree::find(std::vector<std::string> &nameParts) const
	{
		if (nameParts.empty() )
		{
			return app_sets;
		}
		else
		{
			std::string part = nameParts.back();

			nameParts.pop_back();

			auto it = list.find(part);

			if (list.end() == it)
			{
				it = list.find("*");

				if (list.end() != it)
				{
					return app_sets;
				}
			}
			else
			{
				return it->second->find(nameParts);
			}

			return nullptr;
		}
	}

	ServerApplicationSettings *ServerApplicationsTree::find(const std::string &name) const
	{
		std::vector<std::string> name_parts;

		size_t delimiter = name.find('.');

		if (std::string::npos != delimiter)
		{
			size_t cur_pos = 0;

			while (std::string::npos != delimiter)
			{
				std::string part = name.substr(cur_pos, delimiter - cur_pos);
				name_parts.emplace_back(std::move(part) );
				cur_pos = delimiter + 1;
				delimiter = name.find('.', cur_pos);
			}

			std::string part = name.substr(cur_pos);
			name_parts.emplace_back(std::move(part) );
		}
		else
		{
			name_parts.emplace_back(name);
		}

		return find(name_parts);
	}

	void ServerApplicationsTree::collectApplicationSettings(std::unordered_set<ServerApplicationSettings *> &set) const
	{
		for (auto l : list)
		{
			if (nullptr != l.second->app_sets)
			{
				set.insert(l.second->app_sets);
			}

			l.second->collectApplicationSettings(set);
		}
	}

	void ServerApplicationsTree::clear()
	{
		if (false == list.empty() )
		{
			for (auto &it : list)
			{
				delete it.second;
			}

			list.clear();
		}
	}
};