
#include "System.h"

#include <array>

#ifdef WIN32
	#include <tchar.h>
	::TCHAR myWndClassName[] = TEXT("WndClassNameConstant");

	#ifdef UNICODE
		#include <codecvt>
	#endif
#endif

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
			std::array<::TCHAR, 257> class_name;

			::GetClassName(hWnd, class_name.data(), class_name.size() - 1);

			if (0 == ::_tcscmp(class_name.data(), myWndClassName) )
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

		::EnumWindows(EnumProc, reinterpret_cast<::LPARAM>(&ed) );

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
		std::array<TCHAR, MAX_PATH + 1> buf;

		const size_t len = ::GetTempPath(buf.size(), buf.data() );

		#ifdef UNICODE
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			return converter.to_bytes(buf.data() );
		#else
			return std::string(buf.cbegin(), buf.cbegin() + len);
		#endif
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

	bool isFileExists(const std::string &fileName)
	{
	#ifdef WIN32
		#ifdef UNICODE
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			const std::wstring file_name = converter.from_bytes(fileName);
		#else
			const std::string &file_name = fileName;
		#endif

		const ::DWORD attrib = ::GetFileAttributes(file_name.c_str() );

		if (INVALID_FILE_ATTRIBUTES == attrib)
		{
			return false;
		}

		return FILE_ATTRIBUTE_DIRECTORY != (attrib & FILE_ATTRIBUTE_DIRECTORY);
	#elif POSIX
		struct ::stat attrib;

		if (-1 == ::stat(fileName.c_str(), &attrib) )
		{
			return false;
		}

		return S_ISREG(attrib.st_mode);
	#else
		#error "Undefine platform"
	#endif
	}

	bool getFileSizeAndTimeGmt(const std::string &filePath, size_t *fileSize, time_t *fileTime)
	{
	#ifdef WIN32
		#ifdef UNICODE
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			const std::wstring file_path = converter.from_bytes(filePath);
		#else
			const std::string &file_path = filePath;
		#endif

		::HANDLE hFile = ::CreateFile(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);

		if (INVALID_HANDLE_VALUE == hFile)
		{
			return false;
		}

		if (false == ::GetFileSizeEx(hFile, reinterpret_cast<::PLARGE_INTEGER>(fileSize) ) )
		{
			::CloseHandle(hFile);
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