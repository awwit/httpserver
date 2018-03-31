#pragma once

#ifdef WIN32
	#include <wtypes.h>
	#include <WinDef.h>
#elif POSIX
	#include <semaphore.h>
#endif

#include <string>

namespace System
{
	class GlobalMutex
	{
	private:
	#ifdef WIN32
		::HANDLE mtx_desc;
	#elif POSIX
		::sem_t *mtx_desc;
	#else
		#error "Undefined platform"
	#endif

		std::string mtx_name;

	public:
		GlobalMutex() noexcept;
		~GlobalMutex() noexcept;

		bool create(const std::string &mutexName);
		bool destory();

		static bool destory(const std::string &mutexName);

		bool open(const std::string &mutexName);
		bool close() noexcept;

		bool is_open() const noexcept;

		bool lock() const noexcept;
		bool try_lock() const noexcept;
		bool unlock() const noexcept;
	};
}
