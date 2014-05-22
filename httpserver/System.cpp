
#include "System.h"

namespace System
{
#ifdef WIN32
	struct EnumData
	{
		native_processid_type process_id;
		::HWND hWnd;
	};

	BOOL WINAPI EnumProc(::HWND hWnd, ::LPARAM lParam)
	{
		EnumData &ed = *reinterpret_cast<EnumData *>(lParam);

		native_processid_type process_id = 0;

		::GetWindowThreadProcessId(hWnd, &process_id);

		if (process_id == ed.process_id)
		{
			ed.hWnd = hWnd;

			return false;
		}

		return true;
	}
#endif

	bool sendSignal(const native_processid_type pid, const int signal)
	{
	#ifdef WIN32
		EnumData ed = {pid, nullptr};

		if (0 == ::EnumWindows(EnumProc, reinterpret_cast<LPARAM>(&ed) ) )
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
			0
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