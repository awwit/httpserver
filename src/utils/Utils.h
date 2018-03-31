#pragma once

#include <locale>
#include <vector>
#include <unordered_map>

namespace Utils
{
	void toLower(std::string &str) noexcept;
	std::string getLowerString(const std::string &str);

	void trim(std::string &str);
	std::string getTrimmedString(const std::string &str);

	std::vector<std::string> explode(const std::string &str, const char sep);

	std::string encodeHtmlSymbols(const std::string &str);

	std::string binToHexString(const void *bin, const size_t size);

	std::string hexStringToBin(const std::string &hexStr);

	uint64_t hton64(const uint64_t host64) noexcept;
	uint64_t ntoh64(const uint64_t net64) noexcept;

	void hton24(void *dest, const uint32_t host32) noexcept;
	uint32_t ntoh24(const void *src24) noexcept;

	std::string getUniqueName();

	size_t getPackNumberSize(const size_t number) noexcept;
	size_t getPackStringSize(const std::string &str) noexcept;

	template<typename T>
	size_t getPackContainerSize(const T &container) noexcept
	{
		size_t full_size = getPackNumberSize(container.size() );

		for (auto const &pair : container) {
			full_size += getPackStringSize(pair.first);
			full_size += getPackStringSize(pair.second);
		}

		return full_size;
	}

	uint8_t *packPointer(uint8_t *dest, void *pointer) noexcept;
	uint8_t *packNumber(uint8_t *dest, const size_t number) noexcept;
	uint8_t *packString(uint8_t *dest, const std::string &str) noexcept;

	template<typename T>
	uint8_t *packContainer(void *dest, const T &container) noexcept
	{
		uint8_t *addr = reinterpret_cast<uint8_t *>(dest);

		addr = packNumber(addr, container.size() );

		for (auto const &pair : container) {
			addr = packString(addr, pair.first);
			addr = packString(addr, pair.second);
		}

		return addr;
	}

	void packPointer(std::vector<char> &buf, void *pointer);
	void packNumber(std::vector<char> &buf, const size_t number);
	void packString(std::vector<char> &buf, const std::string &str);

	template<typename T>
	void packContainer(std::vector<char> &buf, const T &container)
	{
		packNumber(buf, container.size() );

		for (auto const &pair : container) {
			packString(buf, pair.first);
			packString(buf, pair.second);
		}
	}

	const uint8_t *unpackPointer(void **pointer, const uint8_t *src) noexcept;
	const uint8_t *unpackNumber(size_t *number, const uint8_t *src) noexcept;
	const uint8_t *unpackString(std::string &str, const uint8_t *src);

	template<typename T>
	const uint8_t *unpackContainer(T &container, const uint8_t *src)
	{
		size_t count;
		src = unpackNumber(&count, src);

		for (size_t i = 0; i < count; ++i) {
			std::string key;
			src = unpackString(key, src);

			std::string value;
			src = unpackString(value, src);

			container.emplace(std::move(key), std::move(value) );
		}

		return src;
	}

	template<typename T>
	const uint8_t *unpackVector(T &vector, const uint8_t *src)
	{
		size_t count;
		src = unpackNumber(&count, src);

		for (size_t i = 0; i < count; ++i) {
			std::string key;
			src = unpackString(key, src);

			std::string value;
			src = unpackString(value, src);

			vector.emplace_back(std::move(key), std::move(value) );
		}

		return src;
	}

	time_t rfc822DatetimeToTimestamp(const std::string &strTime);
	time_t predefinedDatetimeToTimestamp(const char *strTime);

	std::string getDatetimeAsString(time_t tTime = ~0, const bool isGmtTime = false);
	std::string predefinedDatetimeToRfc822(const char *strTime);

	size_t getNumberLength(size_t number) noexcept;

	bool parseCookies(
		const std::string &cookieHeader,
		std::unordered_multimap<std::string, std::string> &cookies
	);

	std::string urlEncode(const std::string &str);
	std::string urlDecode(const std::string &str);
}
