#pragma once

#ifdef WIN32
	#include <wtypes.h>
	#include <WinDef.h>
#elif POSIX
	#include <semaphore.h>
#endif

#include <string>

namespace HttpServer
{
	class GlobalMutex
	{
	private:
	#ifdef WIN32
		::HANDLE mtx_desc;
	#elif POSIX
		::sem_t *mtx_desc;
	#else
		#error "Undefine platform"
	#endif

		std::string mtx_name;

	public:
		GlobalMutex();
		~GlobalMutex();

		bool create(const std::string &mutexName);
		bool destory();

		static bool destory(const std::string &mutexName);

		bool open(const std::string &mutexName);
		bool close();

		bool is_open() const;

		bool lock() const;
		bool try_lock() const;
		bool unlock() const;
	};
};