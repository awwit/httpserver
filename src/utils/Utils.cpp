
#include "Utils.h"

#include <array>
#include <chrono>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <cctype>
#include <algorithm>

namespace Utils
{
	void toLower(std::string &str) noexcept {
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	}

	std::string getLowerString(const std::string &str) {
		std::string copy = str;
		toLower(copy);
		return copy;
	}

	void trim(std::string &str)
	{
		static const std::array<char, 7> whitespace { " \t\n\v\f\r" };

		const size_t last = str.find_last_not_of(whitespace.data() );

		if (std::string::npos == last) {
			return str.clear();
		}

		str.assign(
			str,
			str.find_first_not_of(whitespace.data() ),
			last + 1
		);
	}

	std::string getTrimmedString(const std::string &str) {
		std::string copy = str;
		trim(copy);
		return copy;
	}

	std::vector<std::string> explode(const std::string &str, const char sep)
	{
		std::vector<std::string> values;

		for (size_t pos = 0; std::string::npos != pos;)
		{
			const size_t delimiter = str.find(sep, pos);

			std::string value = str.substr(pos, delimiter - pos);
			trim(value);

			values.emplace_back(std::move(value) );

			pos = delimiter;

			if (std::string::npos != pos) {
				++pos;
			}
		}

		return values;
	}

	std::string encodeHtmlSymbols(const std::string &str)
	{
		std::string buf;
		buf.reserve(str.length() );

		for (size_t pos = 0; pos < str.length(); ++pos) {
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

		for (size_t i = dataSize - 1; std::numeric_limits<size_t>::max() != i; --i) {
			str[i * 2 + 0] = hexDigits[bin[i] >> 4];
			str[i * 2 + 1] = hexDigits[bin[i] & 0x0F];
		}

		return str;
	}

	static unsigned char hexStringToBinEncodeSymbol(const char c) noexcept
	{
		if (c >= '0' && c <= '9') {
			return static_cast<unsigned char>(c - 0x30);
		}
		else if (c >= 'a' && c <= 'f') {
			return static_cast<unsigned char>(c - 0x57);
		}
		else if (c >= 'A' && c <= 'F') {
			return static_cast<unsigned char>(c - 0x37);
		}

		return 0;
	}

	std::string hexStringToBin(const std::string &hexStr)
	{
		std::string bin(hexStr.length() / 2, 0);

		for (size_t i = 0; i < bin.length(); ++i) {
			const char a = hexStr[i * 2 + 0];
			const char b = hexStr[i * 2 + 1];

			bin[i] = char(
				(hexStringToBinEncodeSymbol(a) << 4) | hexStringToBinEncodeSymbol(b)
			);
		}

		return bin;
	}

	enum Endianness {
		INIT = 0,
		LITE = 1,
		BIGE = 2
	};

	uint64_t hton64(const uint64_t host64) noexcept
	{
		static Endianness endian = Endianness::INIT;

		union {
			uint64_t ull;
			unsigned char c[sizeof(uint64_t)];
		} x;

		if (endian == Endianness::INIT) {
			x.ull = 0x01;
			endian = (x.c[7] == 0x01ULL) ? Endianness::BIGE : Endianness::LITE;
		}

		if (endian == Endianness::BIGE) {
			return host64;
		}

		x.ull = host64;

		unsigned char c;

		c = x.c[0]; x.c[0] = x.c[7]; x.c[7] = c;
		c = x.c[1]; x.c[1] = x.c[6]; x.c[6] = c;
		c = x.c[2]; x.c[2] = x.c[5]; x.c[5] = c;
		c = x.c[3]; x.c[3] = x.c[4]; x.c[4] = c;

		return x.ull;
	}

	uint64_t ntoh64(const uint64_t net64) noexcept {
		return hton64(net64);
	}

	void hton24(void *dest, const uint32_t src) noexcept
	{
		static Endianness endian = Endianness::INIT;

		union {
			uint32_t ui;
			uint8_t c[sizeof(uint32_t)];
		} x;

		if (endian == Endianness::INIT) {
			x.ui = 0x01;
			endian = (x.c[3] == 0x01) ? Endianness::BIGE : Endianness::LITE;
		}

		x.ui = src;

		if (endian == Endianness::BIGE) {
			x.ui <<= 8;
		} else {
			uint8_t c = x.c[0];
			x.c[0] = x.c[2];
			x.c[2] = c;
		}

		std::copy(x.c, x.c + 3, reinterpret_cast<uint8_t *>(dest) );
	}

	uint32_t ntoh24(const void *src24) noexcept
	{
		static Endianness endian = Endianness::INIT;

		union {
			uint32_t ui;
			uint8_t c[sizeof(uint32_t)];
		} x;

		if (endian == Endianness::INIT) {
			x.ui = 0x01;
			endian = (x.c[3] == 0x01) ? Endianness::BIGE : Endianness::LITE;
		}

		if (endian == Endianness::BIGE) {
			return *reinterpret_cast<const uint32_t *>(src24) >> 8;
		}

		const uint8_t *addr = reinterpret_cast<const uint8_t *>(src24);

		x.ui = 0;

		x.c[0] = addr[2];
		x.c[1] = addr[1];
		x.c[2] = addr[0];

		return x.ui;
	}

	std::string getUniqueName() {
		size_t time = size_t(
			std::chrono::high_resolution_clock::now().time_since_epoch().count()
		);

		time = hton64(time);

		return binToHexString(&time, sizeof(time) );
	}

	constexpr uint8_t PACK_NUMBER_SIZE_BYTE = 252;
	constexpr uint8_t PACK_NUMBER_SIZE_16 = 253;
	constexpr uint8_t PACK_NUMBER_SIZE_32 = 254;
	constexpr uint8_t PACK_NUMBER_SIZE_MAX = 255;

	size_t getPackNumberSize(const size_t number) noexcept
	{
		if (number <= PACK_NUMBER_SIZE_BYTE) {
			return sizeof(uint8_t);
		}
		else if (number <= std::numeric_limits<uint16_t>::max() ) {
			return sizeof(uint8_t) + sizeof(uint16_t);
		}
		else if (number <= std::numeric_limits<uint32_t>::max() ) {
			return sizeof(uint8_t) + sizeof(uint32_t);
		}

		return sizeof(uint8_t) + sizeof(size_t);
	}

	size_t getPackStringSize(const std::string &str) noexcept {
		return getPackNumberSize(str.length() ) + str.length();
	}

	uint8_t *packPointer(uint8_t *dest, void *pointer) noexcept {
		*reinterpret_cast<void **>(dest) = pointer;
		return dest + sizeof(void *);
	}

	uint8_t *packNumber(uint8_t *dest, const size_t number) noexcept
	{
		if (number <= PACK_NUMBER_SIZE_BYTE) {
			*dest = static_cast<uint8_t>(number);

			dest += sizeof(uint8_t);
		}
		else if (number <= std::numeric_limits<uint16_t>::max() ) {
			*dest = PACK_NUMBER_SIZE_16;

			dest += sizeof(uint8_t);

			*reinterpret_cast<uint16_t *>(dest) = static_cast<uint16_t>(number);

			dest += sizeof(uint16_t);
		}
		else if (number <= std::numeric_limits<uint32_t>::max() ) {
			*dest = PACK_NUMBER_SIZE_32;

			dest += sizeof(uint8_t);

			*reinterpret_cast<uint32_t *>(dest) = static_cast<uint32_t>(number);

			dest += sizeof(uint32_t);
		} else {
			*dest = PACK_NUMBER_SIZE_MAX;

			dest += sizeof(uint8_t);

			*reinterpret_cast<size_t *>(dest) = number;

			dest += sizeof(size_t);
		}

		return dest;
	}

	uint8_t *packString(uint8_t *dest, const std::string &str) noexcept {
		dest = packNumber(dest, str.length() );
		std::memcpy(dest, str.data(), str.length() );
		return dest + str.length();
	}

	void packPointer(std::vector<char> &buf, void *pointer) {
		buf.resize(buf.size() + sizeof(void *) );
		uint8_t *dest = reinterpret_cast<uint8_t *>(buf.data() + buf.size() - sizeof(void *) );
		*reinterpret_cast<void **>(dest) = pointer;
	}

	void packNumber(std::vector<char> &buf, const size_t number)
	{
		if (number <= PACK_NUMBER_SIZE_BYTE) {
			buf.emplace_back(number);
		}
		else if (number <= std::numeric_limits<uint16_t>::max() ) {
			buf.emplace_back(PACK_NUMBER_SIZE_16);

			buf.resize(buf.size() + sizeof(uint16_t) );

			*reinterpret_cast<uint16_t *>(buf.data() + buf.size() - sizeof(uint16_t) ) = static_cast<uint16_t>(number);
		}
		else if (number <= std::numeric_limits<uint32_t>::max() ) {
			buf.emplace_back(PACK_NUMBER_SIZE_32);

			buf.resize(buf.size() + sizeof(uint32_t) );

			*reinterpret_cast<uint32_t *>(buf.data() + buf.size() - sizeof(uint32_t) ) = static_cast<uint32_t>(number);
		} else {
			buf.emplace_back(PACK_NUMBER_SIZE_MAX);

			buf.resize(buf.size() + sizeof(size_t) );

			*reinterpret_cast<size_t *>(buf.data() + buf.size() - sizeof(size_t) ) = number;
		}
	}

	void packString(std::vector<char> &buf, const std::string &str)
	{
		packNumber(buf, str.length() );

		if (str.length() ) {
			buf.insert(
				buf.end(),
				str.cbegin(),
				str.cend()
			);
		}
	}

	const uint8_t *unpackPointer(void **pointer, const uint8_t *src) noexcept {
		*pointer = *reinterpret_cast<void **>(
			const_cast<void *>(
				static_cast<const void *>(src)
			)
		);

		return src + sizeof(void *);
	}

	const uint8_t *unpackNumber(size_t *number, const uint8_t *src) noexcept
	{
		*number = *src;

		src += sizeof(uint8_t);

		if (*number <= PACK_NUMBER_SIZE_BYTE) {

		}
		else if (*number == PACK_NUMBER_SIZE_16) {
			*number = *reinterpret_cast<const uint16_t *>(src);
			src += sizeof(uint16_t);
		}
		else if (*number == PACK_NUMBER_SIZE_32) {
			*number = *reinterpret_cast<const uint32_t *>(src);
			src += sizeof(uint32_t);
		} else {
			*number = *reinterpret_cast<const size_t *>(src);
			src += sizeof(size_t);
		}

		return src;
	}

	const uint8_t *unpackString(std::string &str, const uint8_t *src)
	{
		size_t length;
		src = unpackNumber(&length, src);

		str.assign(src, src + length);

		return src + length;
	}

	static const std::unordered_map<std::string, int> map_days {
		{"Mon", 0}, {"Tue", 1}, {"Wed", 2}, {"Thu", 3}, {"Fri", 4}, {"Sat", 5}, {"Sun", 6}
	};

	static const std::unordered_map<std::string, int> map_months {
		{"Jan", 0}, {"Feb", 1}, {"Mar", 2}, {"Apr", 3}, {"May", 4}, {"Jun", 5}, {"Jul", 6}, {"Aug", 7}, {"Sep", 8}, {"Oct", 9}, {"Nov", 10}, {"Dec", 11}
	};

	static const std::unordered_map<std::string, int> map_zones {
		{"GMT", 0}, {"UT", 0},
		{"EST", -5 * 3600}, {"EDT", -4 * 3600}, {"CST", -6 * 3600}, {"CDT", -5 * 3600}, {"MST", -7 * 3600}, {"MDT", -6 * 3600}, {"PST", -8 * 3600}, {"PDT", -7 * 3600},
		{"Z", 0}, {"A", -1 * 3600}, {"M", -12 * 3600}, {"N", 1 * 3600}, {"Y", 12 * 3600}
	};

	/**
	 * Parse RFC 882 (ddd, dd MMM yyyy HH:mm:ss K)
	 */
	time_t rfc822DatetimeToTimestamp(const std::string &strTime) {
		std::tm tc {};

		// Parse RFC 882 (ddd, dd MMM yyyy HH:mm:ss K)

		size_t pos = strTime.find_first_not_of(' ');
		size_t delimiter = strTime.find(',', pos);

		if (std::string::npos == delimiter || delimiter - pos != 3) {
			return ~0;
		}

		const std::string day = strTime.substr(pos, delimiter - pos);

		auto const it_day = map_days.find(day);

		if (map_days.cend() != it_day) {
			tc.tm_wday = it_day->second;
		} else {
			return ~0;
		}

		pos = strTime.find_first_not_of(' ', delimiter + 1);
		delimiter = strTime.find_first_of(' ', pos);

		if (std::string::npos == delimiter) {
			return ~0;
		}

		tc.tm_mday = std::atoi(strTime.data() + pos);

		pos = strTime.find_first_not_of(' ', delimiter + 1);
		delimiter = strTime.find_first_of(' ', pos);

		if (std::string::npos == delimiter || delimiter - pos != 3) {
			return ~0;
		}

		const std::string month = strTime.substr(pos, delimiter - pos);

		auto const it_mon = map_months.find(month);

		if (map_months.cend() != it_mon) {
			tc.tm_mon = it_mon->second;
		} else {
			return ~0;
		}

		pos = strTime.find_first_not_of(' ', delimiter + 1);
		delimiter = strTime.find_first_of(' ', pos);

		if (std::string::npos == delimiter) {
			return ~0;
		}

		tc.tm_year = std::atoi(strTime.data() + pos) - 1900;

		pos = strTime.find_first_not_of(' ', delimiter + 1);
		delimiter = strTime.find_first_of(':', pos);

		if (std::string::npos == delimiter) {
			return ~0;
		}

		tc.tm_hour = std::atoi(strTime.data() + pos);

		pos = strTime.find_first_not_of(' ', delimiter + 1);
		delimiter = strTime.find_first_of(':', pos);

		if (std::string::npos == delimiter) {
			return ~0;
		}

		tc.tm_min = std::atoi(strTime.data() + pos);

		pos = strTime.find_first_not_of(' ', delimiter + 1);
		delimiter = strTime.find_first_of(' ', pos);

		if (std::string::npos == delimiter) {
			return ~0;
		}

		tc.tm_sec = std::atoi(strTime.data() + pos);

		pos = strTime.find_first_not_of(' ', delimiter + 1);
		delimiter = strTime.find_first_of(' ', pos);

		if (std::string::npos == delimiter) {
			delimiter = strTime.length();
		}

		if (std::string::npos == pos || delimiter - pos > 5) {
			return ~0;
		}

		const std::string zone = strTime.substr(pos, delimiter - pos);

		auto const it_zone = map_zones.find(zone);

		int timezone = 0;

		if (map_zones.cend() != it_zone) {
			timezone = it_zone->second;
		}
		else if (zone.length() == 5 && ('+' == zone.front() || '-' == zone.front() ) )
		{
			std::array<char, 4> hours;
			std::array<char, 4> minutes;

			zone.copy(hours.data(), 2, 1);
			zone.copy(minutes.data(), 2, 3);

			timezone = std::atoi(hours.data()) * 3600;
			timezone += std::atoi(minutes.data()) * 60;

			if (zone.front() == '-') {
				timezone *= -1;
			}
		} else {
			return ~0;
		}

		tc.tm_isdst = -1;

		return std::mktime(&tc) - timezone;
	}

	static time_t localToGmt(const time_t timestamp)
	{
	#ifdef WIN32
		std::tm stm {};

		::gmtime_s(&stm, &timestamp);

		return std::mktime(&stm);
	#else
		std::tm stm {};

		::gmtime_r(&timestamp, &stm);

		return std::mktime(&stm);
	#endif
	}

	/**
	 * Convert c-string (__DATE__ " " __TIME__) to std::time_t
	 */
	time_t predefinedDatetimeToTimestamp(const char *strTime)
	{
		std::tm tc {};

		const char *ptrStr = std::strchr(strTime, ' ');

		if (nullptr == ptrStr) {
			return ~0;
		}

		const std::string month(strTime, ptrStr);

		auto const it_mon = map_months.find(month);

		if (map_months.cend() != it_mon) {
			tc.tm_mon = it_mon->second;
		} else {
			return ~0;
		}

		++ptrStr;

		// Fix for MS __DATE__
		if (' ' == *ptrStr) {
			++ptrStr;
		}

		strTime = std::strchr(ptrStr, ' ');

		if (nullptr == strTime) {
			return ~0;
		}

		tc.tm_mday = std::atoi(ptrStr);

		++strTime;

		ptrStr = std::strchr(strTime, ' ');

		if (nullptr == ptrStr) {
			return ~0;
		}

		tc.tm_year = std::atoi(strTime) - 1900;

		++ptrStr;

		strTime = std::strchr(ptrStr, ':');

		if (nullptr == strTime) {
			return ~0;
		}

		tc.tm_hour = std::atoi(ptrStr);

		++strTime;

		ptrStr = std::strchr(strTime, ':');

		if (nullptr == ptrStr) {
			return ~0;
		}

		tc.tm_min = std::atoi(strTime);

		++ptrStr;

		tc.tm_sec = std::atoi(ptrStr);

		return localToGmt(std::mktime(&tc) );
	}

	/**
	 * Convert std::time_t to RFC822 std::string
	 */
	std::string getDatetimeAsString(time_t tTime, const bool isGmtTime)
	{
		std::array<char, 32> buf;

		if (tTime == ~0) {
			std::time(&tTime);
		}

	#ifdef WIN32
		std::tm stm {};

		isGmtTime
			? ::localtime_s(&stm, &tTime)
			: ::gmtime_s(&stm, &tTime);

		auto const len = std::strftime(
			buf.data(),
			buf.size(),
			"%a, %d %b %Y %H:%M:%S GMT", // RFC 822
			&stm
		);
	#else
		std::tm stm {};

		isGmtTime
			? ::localtime_r(&tTime, &stm)
			: ::gmtime_r(&tTime, &stm);

		auto const len = std::strftime(
			buf.data(),
			buf.size(),
			"%a, %d %b %G %H:%M:%S GMT", // RFC 822
			&stm
		);
	#endif

		return std::string(buf.data(), buf.data() + len);
	}

	std::string predefinedDatetimeToRfc822(const char *strTime) {
		const std::time_t time = predefinedDatetimeToTimestamp(strTime);
		return getDatetimeAsString(time, false);
	}

	size_t getNumberLength(size_t number) noexcept
	{
		size_t length = 0;

		do {
			++length;
			number /= 10;
		}
		while (number);

		return length;
	}

	bool parseCookies(
		const std::string &cookieHeader,
		std::unordered_multimap<std::string, std::string> &cookies
	) {
		if (cookieHeader.empty() ) {
			return true;
		}

		for (
			size_t cur_pos = 0, next_value;
			std::string::npos != cur_pos;
			cur_pos = next_value
		) {
			next_value = cookieHeader.find(';', cur_pos);

			size_t delimiter = cookieHeader.find('=', cur_pos);

			if (std::string::npos == delimiter || delimiter > next_value) {
				return false;
			}

			std::string key = cookieHeader.substr(
				cur_pos,
				delimiter - cur_pos
			);

			trim(key);
			key = urlDecode(key);

			++delimiter;

			std::string value = cookieHeader.substr(
				delimiter,
				std::string::npos != next_value
					? next_value - delimiter
					: next_value
			);

			trim(value);
			value = urlDecode(value);

			cookies.emplace(
				std::move(key),
				std::move(value)
			);

			if (std::string::npos != next_value) {
				++next_value;
			}
		}

		return true;
	}

	static inline bool isCharUrlAllowed(const char c) noexcept {
		return c == '-' || c == '_' || c == '.' || c == '~';
	}

	std::string urlEncode(const std::string &str)
	{
		std::string encoded;

		static const std::array<char, 17> hexDigits { "0123456789ABCDEF" };

		for (size_t i = 0; i < str.length(); ++i)
		{
			const unsigned char c = static_cast<unsigned char>(str[i]);

			if (std::isalnum(c) || isCharUrlAllowed(char(c) ) ) {
				encoded.push_back(char(c) );
			}
			else if (' ' == c) {
				encoded.push_back('+');
			} else {
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
			unsigned char c = static_cast<unsigned char>(str[i]);

			if ('%' == c) {
				if (i + 2 < str.length() ) {
					const char a = str[++i];
					const char b = str[++i];

					c = static_cast<unsigned char>(
						(hexStringToBinEncodeSymbol(a) << 4) | hexStringToBinEncodeSymbol(b)
					);
				}
			}
			else if ('+' == c) {
				c = ' ';
			}

			decoded.push_back(char(c) );
		}

		return decoded;
	}
}
