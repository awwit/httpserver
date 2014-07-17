
#include "Utils.h"

#include <cstring>
#include <iomanip>

namespace Utils
{
	void trim(std::string &str)
	{
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
			::strcpy_s(s, length + 1, str.c_str() );
		#elif POSIX
			::strcpy(s, str.c_str() );
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

				const HttpServer::FileIncoming &file = it->second;

				arr[i].file_name = stlStringToPChar(file.getName() );
				arr[i].file_type = stlStringToPChar(file.getType() );
				arr[i].file_size = file.getSize();
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

		const size_t str_mon_length = 32;
		char *s_mon = new char[str_mon_length];
		::memset(s_mon, 0, str_mon_length);

		struct ::tm tc = {0};

		// Parse RFC 822
	#ifdef WIN32
		if (std::numeric_limits<int>::max() != ::sscanf_s(strTime.c_str(), "%*s %d %3s %d %d:%d:%d", &tc.tm_mday, s_mon, str_mon_length, &tc.tm_year, &tc.tm_hour, &tc.tm_min, &tc.tm_sec) )
	#else
		if (std::numeric_limits<int>::max() != ::sscanf(strTime.c_str(), "%*s %d %3s %d %d:%d:%d", &tc.tm_mday, s_mon, &tc.tm_year, &tc.tm_hour, &tc.tm_min, &tc.tm_sec) )
	#endif
		{
			tc.tm_year -= 1900;

			auto it_mon = map_months.find(s_mon);

			if (map_months.end() != it_mon)
			{
				tc.tm_mon = it_mon->second;
			}
		}

		delete[] s_mon;

		return ::mktime(&tc);
	}

	std::string getDatetimeStringValue(const ::time_t tTime, const bool isGmtTime)
	{
		char buf[64];

		::time_t cur_time = tTime;

		if ( (time_t)~0 == tTime)
		{
			::time(&cur_time);
		}

	#ifdef WIN32
		struct ::tm stm = {0};

		if (isGmtTime)
		{
			::localtime_s(&stm, &cur_time);
		}
		else
		{
			::gmtime_s(&stm, &cur_time);
		}

		// RFC 822
		::strftime(buf, 64, "%a, %d %b %Y %H:%M:%S GMT", &stm);
	#else
		struct ::tm *ptm = isGmtTime ? localtime(&cur_time) : gmtime(&cur_time);

		// RFC 822
		::strftime(buf, 64, "%a, %d %b %G %H:%M:%S GMT", ptm);
	#endif

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

	bool parseCookies(const std::string &cookieHeader, std::unordered_multimap<std::string, std::string> &cookies)
	{
		if (cookieHeader.empty() )
		{
			return false;
		}

		for (size_t cur_pos = 0, next_value; std::string::npos != cur_pos; cur_pos = next_value)
		{
			next_value = cookieHeader.find(';', cur_pos);

			size_t delimiter = cookieHeader.find('=', cur_pos);

			if (std::string::npos == delimiter || delimiter > next_value)
			{
				return false;
			}

			std::string key = cookieHeader.substr(cur_pos, delimiter - cur_pos);
			trim(key);
			key = urlDecode(key);

			++delimiter;

			std::string value = cookieHeader.substr(delimiter, std::string::npos != next_value ? next_value - delimiter : next_value);
			trim(value);
			value = urlDecode(value);

			cookies.emplace(std::move(key), std::move(value) );

			if (std::string::npos != next_value)
			{
				++next_value;
			}
		}

		return true;
	}

	inline bool isUrlAllowed(const std::string::value_type c)
	{
		static const std::string special("-_.~");

		return std::string::npos != special.find(c);
	}

	std::string urlEncode(const std::string &str)
	{
		std::ostringstream encoded;
		encoded.fill('0');
		encoded << std::hex;

		for (auto it = str.cbegin(); str.cend() != it; ++it)
		{
			const std::string::value_type &c = *it;

			if (' ' == c)
			{
				encoded << '+';
			}
			else if (std::isalnum(c) || isUrlAllowed(c) )
			{
				encoded << c;
			}
			else
			{
				encoded << '%' << std::setw(2) << (int) ( (unsigned char) c);
			}
		}

		return encoded.str();
	}

	std::string urlDecode(const std::string &str)
	{
		std::string decoded;

		std::string::value_type ch[3] = {0};

		for (auto it = str.cbegin(); str.cend() != it; ++it)
		{
			const std::string::value_type &c = *it;

			if ('%' == c)
			{
				++it;

				if (str.cend() == it)
				{
					break;
				}

				ch[0] = *it;

				++it;

				if (str.cend() == it)
				{
					break;
				}

				ch[1] = *it;

				decoded.push_back(strtoul(ch, nullptr, 16) );
			}
			else if ('+' == c)
			{
				decoded.push_back(' ');
			}
			else
			{
				decoded.push_back(c);
			}
		}

		return decoded;
	}
};