#pragma once

#include "RawData.h"
#include "FileIncoming.h"

#include <locale>
#include <string>
#include <algorithm>
#include <functional>
#include <sstream>
#include <chrono>
#include <map>
#include <unordered_map>
#include <cctype>

namespace Utils
{
	inline void tolower(std::string &str, const std::locale &loc)
	{
		for (auto &c : str)
		{
			c = std::tolower(c, loc);
		}
	}

	void trim(std::string &str);

	std::vector<std::string> explode(const std::string &str, const char sep);

	inline std::string getUniqueName()
	{
		std::stringstream s;
		s << std::hex << std::chrono::high_resolution_clock::now().time_since_epoch().count();
		return s.str();
	}

	char *stlStringToPChar(const std::string &str);

	template<typename T>
	void stlToRawPairs(Utils::raw_pair *raw[], const T &stl)
	{
		if (raw && stl.size() )
		{
			raw_pair *arr = new raw_pair[stl.size()];

			*raw = arr;

			size_t i = 0;

			for (auto it = stl.cbegin(); stl.cend() != it; ++it, ++i)
			{
				arr[i].key = stlStringToPChar(it->first);
				arr[i].value = stlStringToPChar(it->second);
			}
		}
	}

	template<typename T>
	void rawPairsToStl(T &stl, const Utils::raw_pair raw[], const size_t count)
	{
		for (size_t i = 0; i < count; ++i)
		{
			stl.emplace(raw[i].key ? raw[i].key : "", raw[i].value ? raw[i].value : "");
		}
	}

	void filesIncomingToRawFilesInfo(Utils::raw_fileinfo *raw[], const std::unordered_multimap<std::string, HttpServer::FileIncoming> &map);
	void rawFilesInfoToFilesIncoming(std::unordered_multimap<std::string, HttpServer::FileIncoming> &map, const Utils::raw_fileinfo raw[], const size_t count);

	void destroyRawPairs(Utils::raw_pair raw[], const size_t count);
	void destroyRawFilesInfo(Utils::raw_fileinfo raw[], const size_t count);

	time_t stringTimeToTimestamp(const std::string &strTime);

	std::string getDatetimeAsString(const ::time_t tTime = ~0, const bool isGmtTime = false);

	size_t getNumberLength(const size_t number);

	bool parseCookies(const std::string &cookieHeader, std::unordered_multimap<std::string, std::string> &cookies);

	std::string urlEncode(const std::string &str);
	std::string urlDecode(const std::string &str);
};