#pragma once

#ifdef WIN32
	#include <Windows.h>
	char myWndClassName[];

    #ifdef SIGTERM
        #undef SIGTERM
        #define SIGTERM (WM_USER + 15)
    #endif

    #ifdef SIGINT
        #undef SIGINT
        #define SIGINT (WM_USER + 2)
    #endif

    #ifndef SIGUSR1
        #define SIGUSR1 (WM_USER + 10)
    #endif

    #ifndef SIGUSR2
        #define SIGUSR2 (WM_USER + 12)
    #endif
#elif POSIX
	#include <csignal>
	#include <sys/sysinfo.h>
	#include <sys/stat.h>
	#include <unistd.h>
#else
	#error "Undefine platform"
#endif

#include <vector>
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

	std::string getTempDir();

	bool isFileExists(const std::string &fileName);

	bool getFileSizeAndTimeGmt(const std::string &filePath, size_t *fileSize, time_t *fileTime);
};