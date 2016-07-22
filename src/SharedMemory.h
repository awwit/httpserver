#pragma once

#ifdef WIN32
	#include <wtypes.h>
	#include <WinDef.h>
#endif

#include <string>

namespace HttpServer
{
	class SharedMemory
	{
	private:
	#ifdef WIN32
		::HANDLE shm_desc;
	#elif POSIX
		int shm_desc;
	#else
		#error "Undefine platform"
	#endif

		std::string shm_name;

	public:
		SharedMemory();
		~SharedMemory();

		bool create(const std::string &memName, const size_t memSize);
		bool open(const std::string &memName);

		bool is_open() const;

		bool write(const void *data, const size_t size, const size_t offset = 0) const;
		bool read(void *dest, const size_t size, const size_t offset = 0) const;

		bool close();
		bool destroy();

		static bool destroy(const std::string &memName);
	};
};