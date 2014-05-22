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

#ifndef SIGUSR1
	#define SIGUSR1 10
#endif

#include <string>
#include <ctime>
#include <thread>

namespace System
{
#ifdef WIN32
	typedef ::SOCKET native_socket_type;
#elif POSIX
	typedef int native_socket_type;
#else
	#error "Undefine platform"
#endif

#ifdef WIN32
	typedef ::DWORD native_processid_type;
#elif POSIX
	typedef ::pid_t native_processid_type;
#else
	#error "Undefine platform"
#endif

	inline size_t getProcessorsCount()
	{
	#ifdef WIN32
		::SYSTEM_INFO si = {0};
		::GetSystemInfo(&si);
		return si.dwNumberOfProcessors;
	#elif POSIX
		return ::get_nprocs();
	#else
		#error "Undefine platform"
	#endif
	}

	inline native_processid_type getProcessId()
	{
	#ifdef WIN32
		return ::GetCurrentProcessId();
	#elif POSIX
		return ::getpid();
	#else
		#error "Undefine platform"
	#endif
	}

	bool sendSignal(const native_processid_type pid, const int signal);

	inline bool isDoneThread(const std::thread::native_handle_type handle)
	{
	#ifdef WIN32
		return WAIT_OBJECT_0 == ::WaitForSingleObject(handle, 0);
	#elif POSIX
		return 0 != ::pthread_kill(handle, 0);
	#else
		#error "Undefine platform"
	#endif
	}

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