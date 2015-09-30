
#include "System.h"

namespace System
{
#ifdef WIN32
	struct EnumData
	{
		native_processid_type process_id;
		::HWND hWnd;
	};

    ::BOOL WINAPI EnumProc(::HWND hWnd, ::LPARAM lParam)
	{
		EnumData &ed = *reinterpret_cast<EnumData *>(lParam);

		native_processid_type process_id = 0;

		::GetWindowThreadProcessId(hWnd, &process_id);

        if (process_id == ed.process_id && ::GetConsoleWindow() != hWnd)
		{
			char class_name[65] = {0};

            ::GetClassName(hWnd, class_name, 64);

            if (0 == ::strcmp(class_name, myWndClassName) )
			{
				ed.hWnd = hWnd;

				return false;
			}
		}

		return true;
	}
#endif

	bool sendSignal(const native_processid_type pid, const int signal)
	{
	#ifdef WIN32
		EnumData ed = {pid, 0};

		::EnumWindows(EnumProc, reinterpret_cast<LPARAM>(&ed) );

		if (0 == ed.hWnd)
		{
			return false;
		}

		return 0 != ::PostMessage(ed.hWnd, signal, 0, 0);
	#elif POSIX
		return 0 == ::kill(pid, signal);
	#else
		#error "Undefine platform"
	#endif
	}

	std::string getTempDir()
	{
	#ifdef WIN32
		std::vector<std::string::value_type> buf(MAX_PATH + 1);

		const size_t len = ::GetTempPath(MAX_PATH + 1, buf.data() );

		return std::string(buf.cbegin(), buf.cbegin() + len);
	#elif POSIX
        const char *buf = ::getenv("TMPDIR");

		if (nullptr == buf)
		{
			return std::string("/tmp/");
		}

		std::string str(buf);

		if ('/' != str.back() )
		{
			str.push_back('/');
		}

		return str;
	#else
		#error "Undefine platform"
	#endif
	}

	bool getFileSizeAndTimeGmt(const std::string &filePath, size_t *fileSize, time_t *fileTime)
	{
	#ifdef WIN32
		::HANDLE hFile = ::CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);

		if (INVALID_HANDLE_VALUE == hFile)
		{
			return false;
		}

		if (false == ::GetFileSizeEx(hFile, reinterpret_cast<::PLARGE_INTEGER>(fileSize) ) )
		{
			return false;
		}

		::FILETIME ftWrite;

		::BOOL result = ::GetFileTime(hFile, nullptr, nullptr, &ftWrite);

		::CloseHandle(hFile);

		if (false == result)
		{
			return false;
		}

		::SYSTEMTIME stUtc;

		::FileTimeToSystemTime(&ftWrite, &stUtc);

		struct ::tm tm_time {
			stUtc.wSecond,
			stUtc.wMinute,
			stUtc.wHour,
			stUtc.wDay,
			stUtc.wMonth - 1,
			stUtc.wYear - 1900,
			0,
			0,
            -1
		};

		*fileTime = ::mktime(&tm_time);

		return true;
	#elif POSIX
		struct ::tm *clock;
		struct ::stat attrib;

		if (-1 == ::stat(filePath.c_str(), &attrib) )
		{
			return false;
		}

		*fileSize = attrib.st_size;

		clock = ::gmtime(&(attrib.st_mtime) );

		*fileTime = ::mktime(clock);

		return true;
	#else
		#error "Undefine platform"
	#endif
	}
};