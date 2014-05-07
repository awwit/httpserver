#pragma once

#ifdef WIN32
	#include <Windows.h>
#elif POSIX
	#include <csignal>
	#include <sys/sysinfo.h>
	#include <sys/stat.h>
	#include <unistd.h>
#else
	#error "Undefine platform"
#endif

#include <string>
#include <ctime>

namespace System
{
#ifdef WIN32
	typedef SOCKET native_socket_type;
#elif POSIX
	typedef int native_socket_type;
#else
	#error "Undefine platform"
#endif

#ifdef WIN32
	typedef DWORD native_processid_type;
#elif POSIX
	typedef pid_t native_processid_type;
#else
	#error "Undefine platform"
#endif

	inline size_t getProcessorsCount()
	{
	#ifdef WIN32
		SYSTEM_INFO si = {0};
		GetSystemInfo(&si);
		return si.dwNumberOfProcessors;
	#elif POSIX
		return get_nprocs();
	#else
		#error "Undefine platform"
	#endif
	}

	inline native_processid_type getProcessId()
	{
	#ifdef WIN32
		return GetCurrentProcessId();
	#elif POSIX
		return getpid();
	#else
		#error "Undefine platform"
	#endif
	}

	bool sendSignal(native_processid_type pid, int signal);

	inline std::string getTempDir()
	{
	#ifdef WIN32
		return std::string("C:/Temp/"); // FIXME: Windows temp dir
	#elif POSIX
		return std::string("/tmp/");
	#else
		#error "Undefine platform"
	#endif
	}

	bool getFileSizeAndTimeGmt(const std::string &, size_t *, time_t *);
};