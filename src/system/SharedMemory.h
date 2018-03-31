#pragma once

#ifdef WIN32
	#include <wtypes.h>
	#include <WinDef.h>
#endif

#include <string>

namespace System
{
	class SharedMemory
	{
	private:
	#ifdef WIN32
		::HANDLE shm_desc;
	#elif POSIX
		int shm_desc;
	#else
		#error "Undefined platform"
	#endif

		std::string shm_name;

	public:
		SharedMemory() noexcept;
		~SharedMemory() noexcept;

		bool create(
			const std::string &memName,
			const size_t memSize
		);

		bool open(const std::string &memName);

		bool is_open() const noexcept;

		bool write(
			const void *data,
			const size_t size,
			const size_t offset = 0
		) const noexcept;

		bool read(
			void *dest,
			const size_t size,
			const size_t offset = 0
		) const noexcept;

		bool close() noexcept;
		bool destroy();

		static bool destroy(const std::string &memName);
	};
}
