
#include "Utils.h"

#include <cstring>

namespace Utils
{
	void trim(std::string &str)
	{
	//	str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(std::isspace) ) ).base(), str.end() );
	//	str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(std::isspace) ) ) );

		size_t last = str.find_last_not_of(" \t\n\v\f\r");

		if (std::string::npos == last)
		{
			return str.clear();
		}

		str.assign(str.cbegin() + str.find_first_not_of(" \t\n\v\f\r"), str.cbegin() + last + 1);
	}

	char *stlStringToPChar(const std::string &str)
	{
		size_t length = str.length();
		char *s = nullptr;

		if (length)
		{
			s = new char[length + 1];
		#ifdef WIN32
			strcpy_s(s, length + 1, str.c_str() );
		#elif POSIX
			strcpy(s, str.c_str() );
			s[length] = '\0';
		#else
			#error "Undefine platform"
		#endif
		}

		return s;
	}

	void stlMapToRawPairs(Utils::raw_pair *raw[], const std::map<std::string, std::string> &map)
	{
		if (raw && map.size() )
		{
			raw_pair *arr = new raw_pair[map.size()];

			*raw = arr;

			size_t i = 0;

			for (auto it = map.cbegin(); map.cend() != it; ++it, ++i)
			{
				arr[i].key = stlStringToPChar(it->first);
				arr[i].value = stlStringToPChar(it->second);
			}
		}
	}

	void stlUnorderedMapToRawPairs(Utils::raw_pair *raw[], const std::unordered_map<std::string, std::string> &map)
	{
		if (raw && map.size() )
		{
			raw_pair *arr = new raw_pair[map.size()];

			*raw = arr;

			size_t i = 0;

			for (auto it = map.cbegin(); map.cend() != it; ++it, ++i)
			{
				arr[i].key = stlStringToPChar(it->first);
				arr[i].value = stlStringToPChar(it->second);
			}
		}
	}

	void stlUnorderedMultimapToRawPairs(Utils::raw_pair *raw[], const std::unordered_multimap<std::string, std::string> &map)
	{
		if (raw && map.size() )
		{
			raw_pair *arr = new raw_pair[map.size()];

			*raw = arr;

			size_t i = 0;

			for (auto it = map.cbegin(); map.cend() != it; ++it, ++i)
			{
				arr[i].key = stlStringToPChar(it->first);
				arr[i].value = stlStringToPChar(it->second);
			}
		}
	}

	void filesIncomingToRawFilesInfo(Utils::raw_fileinfo *raw[], const std::unordered_multimap<std::string, HttpServer::FileIncoming> &map)
	{
		if (raw && map.size() )
		{
			raw_fileinfo *arr = new raw_fileinfo[map.size()];

			*raw = arr;

			size_t i = 0;

			for (auto it = map.cbegin(); map.cend() != it; ++it, ++i)
			{
				arr[i].key = stlStringToPChar(it->first);

				const HttpServer::FileIncoming &info = it->second;

				arr[i].file_name = stlStringToPChar(info.getName() );
				arr[i].file_type = stlStringToPChar(info.getType() );
				arr[i].file_size = info.getSize();
			}
		}
	}

	void rawPairsToStlMap(std::map<std::string, std::string> &map, const Utils::raw_pair raw[], const size_t count)
	{
		for (size_t i = 0; i < count; ++i)
		{
			map.emplace(raw[i].key ? raw[i].key : "", raw[i].value);
		}
	}

	void rawPairsToStlUnorderedMap(std::unordered_map<std::string, std::string> &map, const Utils::raw_pair raw[], const size_t count)
	{
		for (size_t i = 0; i < count; ++i)
		{
			map.emplace(raw[i].key, raw[i].value);
		}
	}

	void rawPairsToStlUnorderedMultimap(std::unordered_multimap<std::string, std::string> &map, const Utils::raw_pair raw[], const size_t count)
	{
		for (size_t i = 0; i < count; ++i)
		{
			map.emplace(raw[i].key, raw[i].value);
		}
	}

	void rawFilesInfoToFilesIncoming(std::unordered_multimap<std::string, HttpServer::FileIncoming> &map, const Utils::raw_fileinfo raw[], const size_t count)
	{
		for (size_t i = 0; i < count; ++i)
		{
			map.emplace(raw[i].key, HttpServer::FileIncoming(raw[i].file_name, raw[i].file_type, raw[i].file_size) );
		}
	}

	void destroyRawPairs(Utils::raw_pair raw[], const size_t count)
	{
		if (raw)
		{
			for (size_t i = 0; i < count; ++i)
			{
				raw_pair &cur = raw[i];

				delete[] cur.key;
				delete[] cur.value;
			}

			delete[] raw;
		}
	}

	void destroyRawFilesInfo(Utils::raw_fileinfo raw[], const size_t count)
	{
		if (raw)
		{
			for (size_t i = 0; i < count; ++i)
			{
				raw_fileinfo &cur = raw[i];

				delete[] cur.key;
				delete[] cur.file_name;
				delete[] cur.file_type;
			}

			delete[] raw;
		}
	}

	time_t stringTimeToTimestamp(const std::string &strTime)
	{
	/*	static const std::unordered_map<std::string, int> map_days {
			{"Sun", 0}, {"Mon", 1}, {"Tue", 2}, {"Wed", 3}, {"Thu", 4}, {"Fri", 5}, {"Sat", 6}
		};*/

		static const std::unordered_map<std::string, int> map_months {
			{"Jan", 0}, {"Feb", 1}, {"Mar", 2}, {"Apr", 3}, {"May", 4}, {"Jun", 5}, {"Jul", 6}, {"Aug", 7}, {"Sep", 8}, {"Oct", 9}, {"Nov", 10}, {"Dec", 11}
		};

		char *s_mon = new char[32];
		memset(s_mon, 0, 32);

		struct tm tc = {0};

		// Parse RFC 822
		if (std::numeric_limits<int>::max() != sscanf(strTime.c_str(), "%*s %d %3s %d %d:%d:%d", &tc.tm_mday, s_mon, &tc.tm_year, &tc.tm_hour, &tc.tm_min, &tc.tm_sec) )
		{
			tc.tm_year -= 1900;

			auto it_mon = map_months.find(s_mon);

			if (map_months.end() != it_mon)
			{
				tc.tm_mon = it_mon->second;
			}
		}

		delete[] s_mon;

		return mktime(&tc);
	}

	std::string getDatetimeStringValue(const time_t tTime, const bool isGmtTime)
	{
		char buf[64];

		time_t cur_time = tTime;

		if (-1 == tTime)
		{
			time(&cur_time);
		}

		struct tm *ptm = isGmtTime ? localtime(&cur_time) : gmtime(&cur_time);

		// RFC 822
		strftime(buf, 64, "%a, %d %b %G %H:%M:%S GMT", ptm);

		return std::string(buf);
	}

	size_t getNumberLength(const size_t number)
	{
		size_t length = 0;

		size_t n = number;

		do
		{
			++length;
			n /= 10;
		}
		while (n);

		return length;
	}
};