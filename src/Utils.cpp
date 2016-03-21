
#include "Utils.h"

#include <array>
#include <chrono>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <cctype>

namespace Utils
{
	void toLower(std::string &str, const std::locale &loc)
	{
		for (auto &c : str)
		{
			c = std::tolower(c, loc);
		}
	}

	void trim(std::string &str)
	{
		static const std::array<char, 7> whitespace { " \t\n\v\f\r" };

		const size_t last = str.find_last_not_of(whitespace.data() );

		if (std::string::npos == last)
		{
			return str.clear();
		}

		str.assign(str.cbegin() + str.find_first_not_of(whitespace.data() ), str.cbegin() + last + 1);
	}

	std::vector<std::string> explode(const std::string &str, const char sep)
	{
		std::vector<std::string> values;

		for (size_t pos = 0; std::string::npos != pos;)
		{
			size_t delimiter = str.find(sep, pos);

			std::string value = str.substr(pos, delimiter);
			trim(value);

			values.emplace_back(std::move(value) );

			pos = delimiter;

			if (std::string::npos != pos)
			{
				++pos;
			}
		}

		return values;
	}

	std::string encodeHtmlSymbols(const std::string &str)
	{
		std::string buf;
		buf.reserve(str.length() );

		for (size_t pos = 0; pos < str.length(); ++pos)
		{
			switch (str[pos])
			{
				case '&': buf.append("&amp;"); break;
				case '\"': buf.append("&quot;"); break;
				case '\'': buf.append("&apos;"); break;
				case '<': buf.append("&lt;"); break;
				case '>': buf.append("&gt;"); break;
				default: buf.push_back(str[pos]); break;
			}
		}

		return buf;
	}

	std::string binToHexString(const void *binData, const size_t dataSize)
	{
		std::string str(dataSize * 2, 0);

		const uint8_t *bin = reinterpret_cast<const uint8_t *>(binData);

		static const std::array<char, 17> hexDigits { "0123456789abcdef" };

		for (size_t i = dataSize - 1; std::numeric_limits<size_t>::max() != i; --i)
		{
			str[i * 2 + 0] = hexDigits[bin[i] >> 4];
			str[i * 2 + 1] = hexDigits[bin[i] & 0x0F];
		}

		return str;
	}

	static unsigned char hexStringToBinEncodeSymbol(const char c)
	{
		if (c >= '0' && c <= '9')
		{
			return c - 0x30;
		}
		else if (c >= 'a' && c <= 'f')
		{
			return c - 0x57;
		}
		else if (c >= 'A' && c <= 'F')
		{
			return c - 0x37;
		}

		return 0;
	}

	std::string hexStringToBin(const std::string &hexStr)
	{
		std::string bin(hexStr.length() / 2, 0);

		for (size_t i = 0; i < bin.length(); ++i)
		{
			const char a = hexStr[i * 2 + 0];
			const char b = hexStr[i * 2 + 1];

			bin[i] = (
				(hexStringToBinEncodeSymbol(a) << 4) | hexStringToBinEncodeSymbol(b)
			);
		}

		return bin;
	}

	unsigned long long htonll(const unsigned long long src)
	{
		enum Endianness {
			INIT = 0,
			LITE = 1,
			BIGE = 2
		};

		static int endian = Endianness::INIT;

		union {
			unsigned long long ull;
			unsigned char c[8];
		} x;

		if (endian == Endianness::INIT)
		{
			x.ull = 0x01;
			endian = (x.c[7] == 0x01ULL) ? Endianness::BIGE : Endianness::LITE;
		}

		if (endian == Endianness::BIGE)
		{
			return src;
		}

		x.ull = src;

		unsigned char c;

		c = x.c[0]; x.c[0] = x.c[7]; x.c[7] = c;
		c = x.c[1]; x.c[1] = x.c[6]; x.c[6] = c;
		c = x.c[2]; x.c[2] = x.c[5]; x.c[5] = c;
		c = x.c[3]; x.c[3] = x.c[4]; x.c[4] = c;

		return x.ull;
	}

	std::string getUniqueName()
	{
		size_t time = std::chrono::high_resolution_clock::now().time_since_epoch().count();

		time = htonll(time);

		return binToHexString(&time, sizeof(time) );
	}

	char *stlStringToPChar(const std::string &str)
	{
		const size_t length = str.length();
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

	void rawFilesInfoToFilesIncoming(std::unordered_multimap<std::string, HttpServer::FileIncoming> &map, const Utils::raw_fileinfo raw[], const size_t count)
	{
		for (size_t i = 0; i < count; ++i)
		{
			map.emplace(raw[i].key ? raw[i].key : "", HttpServer::FileIncoming(raw[i].file_name, raw[i].file_type, raw[i].file_size) );
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
		static const std::unordered_map<std::string, int> map_months {
			{"Jan", 0}, {"Feb", 1}, {"Mar", 2}, {"Apr", 3}, {"May", 4}, {"Jun", 5}, {"Jul", 6}, {"Aug", 7}, {"Sep", 8}, {"Oct", 9}, {"Nov", 10}, {"Dec", 11}
		};

		if (strTime.length() > 64)
		{
			return ~0;
		}

		const size_t str_mon_length = 64;
		std::vector<char> s_mon(str_mon_length);

		struct ::tm tc = {};

		// Parse RFC 822
	#ifdef WIN32
		if (~0 != ::sscanf_s(strTime.c_str(), "%*s %d %3s %d %d:%d:%d", &tc.tm_mday, s_mon.data(), s_mon.size(), &tc.tm_year, &tc.tm_hour, &tc.tm_min, &tc.tm_sec) )
	#else
		if (~0 != ::sscanf(strTime.c_str(), "%*s %d %3s %d %d:%d:%d", &tc.tm_mday, s_mon.data(), &tc.tm_year, &tc.tm_hour, &tc.tm_min, &tc.tm_sec) )
	#endif
		{
			tc.tm_year -= 1900;

			auto it_mon = map_months.find(s_mon.data() );

            if (map_months.cend() != it_mon)
			{
				tc.tm_mon = it_mon->second;
			}
		}

        tc.tm_isdst = -1;

		return ::mktime(&tc);
	}

	std::string getDatetimeAsString(const ::time_t tTime, const bool isGmtTime)
	{
		std::array<char, 64> buf;

		::time_t cur_time = tTime;

		if (tTime == ~0)
		{
			::time(&cur_time);
		}

	#ifdef WIN32
		struct ::tm stm = {};

		isGmtTime ?
			::localtime_s(&stm, &cur_time) :
			::gmtime_s(&stm, &cur_time);

		// RFC 822
		::strftime(buf.data(), buf.size(), "%a, %d %b %Y %H:%M:%S GMT", &stm);
	#else
		struct ::tm *ptm = isGmtTime ?
			::localtime(&cur_time) :
			::gmtime(&cur_time);

		// RFC 822
		::strftime(buf.data(), buf.size(), "%a, %d %b %G %H:%M:%S GMT", ptm);
	#endif

		return std::string(buf.data() );
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
			return true;
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

	static inline bool isCharUrlAllowed(const char c)
	{
		return c == '-' || c == '_' || c == '.' || c == '~';
	}

	std::string urlEncode(const std::string &str)
	{
		std::string encoded;

		static const std::array<char, 17> hexDigits { "0123456789ABCDEF" };

		for (size_t i = 0; i < str.length(); ++i)
		{
			const unsigned char c = str[i];

			if (std::isalnum(c) || isCharUrlAllowed(c) )
			{
				encoded.push_back(c);
			}
			else if (' ' == c)
			{
				encoded.push_back('+');
			}
			else
			{
				const uint8_t a = c >> 4;
				const uint8_t b = c & 0x0F;

				encoded.push_back('%');
				encoded.push_back(hexDigits[a]);
				encoded.push_back(hexDigits[b]);
			}
		}

		return encoded;
	}

	std::string urlDecode(const std::string &str)
	{
		std::string decoded;

		for (size_t i = 0; i < str.length(); ++i)
		{
			unsigned char c = str[i];

			if ('%' == c)
			{
				if (i + 2 < str.length() )
				{
					const char a = str[++i];
					const char b = str[++i];

					c = (
						(hexStringToBinEncodeSymbol(a) << 4) | hexStringToBinEncodeSymbol(b)
					);
				}
			}
			else if ('+' == c)
			{
				c = ' ';
			}

			decoded.push_back(c);
		}

		return decoded;
	}
};