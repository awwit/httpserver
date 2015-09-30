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

	void trim(std::string &);

	inline std::string getUniqueName()
	{
		std::stringstream s;
		s << std::hex << std::chrono::high_resolution_clock::now().time_since_epoch().count();
		return s.str();
	}

	char *stlStringToPChar(const std::string &);

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

	void stlMapToRawPairs(Utils::raw_pair *[], const std::map<std::string, std::string> &);
	void stlMultimapToRawPairs(Utils::raw_pair *[], const std::multimap<std::string, std::string> &);
	void stlUnorderedMapToRawPairs(Utils::raw_pair *[], const std::unordered_map<std::string, std::string> &);
	void stlUnorderedMultimapToRawPairs(Utils::raw_pair *[], const std::unordered_multimap<std::string, std::string> &);
	void filesIncomingToRawFilesInfo(Utils::raw_fileinfo *[], const std::unordered_multimap<std::string, HttpServer::FileIncoming> &);
	void rawPairsToStlMap(std::map<std::string, std::string> &, const Utils::raw_pair [], const size_t);
	void rawPairsToStlMultimap(std::multimap<std::string, std::string> &, const Utils::raw_pair [], const size_t);
	void rawPairsToStlUnorderedMap(std::unordered_map<std::string, std::string> &, const Utils::raw_pair [], const size_t);
	void rawPairsToStlUnorderedMultimap(std::unordered_multimap<std::string, std::string> &, const Utils::raw_pair [], const size_t);
	void rawFilesInfoToFilesIncoming(std::unordered_multimap<std::string, HttpServer::FileIncoming> &, const Utils::raw_fileinfo [], const size_t);
	void destroyRawPairs(Utils::raw_pair [], const size_t);
	void destroyRawFilesInfo(Utils::raw_fileinfo [], const size_t);

	time_t stringTimeToTimestamp(const std::string &);

	std::string getDatetimeAsString(const ::time_t tTime = ~0, const bool isGmtTime = false);

	size_t getNumberLength(const size_t number);

	bool parseCookies(const std::string &, std::unordered_multimap<std::string, std::string> &);

	std::string urlEncode(const std::string &);
	std::string urlDecode(const std::string &);
};