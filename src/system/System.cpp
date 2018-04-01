
#include "System.h"

#include <array>

#ifdef WIN32
	#include <tchar.h>
	::TCHAR myWndClassName[] = TEXT("WndClassNameConstant");

	#ifdef UNICODE
		#include <codecvt>
	#endif
#elif POSIX
	#include <csignal>
	#include <sys/sysinfo.h>
	#include <sys/stat.h>
	#include <unistd.h>
#endif

namespace System
{
#ifdef WIN32
	struct EnumData {
		native_processid_type process_id;
		::HWND hWnd;
	};

	static ::BOOL WINAPI EnumProc(const ::HWND hWnd, const ::LPARAM lParam)
	{
		EnumData &ed = *reinterpret_cast<EnumData * const>(lParam);

		native_processid_type process_id = 0;

		::GetWindowThreadProcessId(hWnd, &process_id);

		if (process_id == ed.process_id && ::GetConsoleWindow() != hWnd)
		{
			std::array<::TCHAR, 257> class_name;

			::GetClassName(hWnd, class_name.data(), int(class_name.size() - 1) );

			if (0 == ::_tcscmp(class_name.data(), myWndClassName) ) {
				ed.hWnd = hWnd;
				return false;
			}
		}

		return true;
	}
#endif

	native_processid_type getProcessId() noexcept
	{
	#ifdef WIN32
		return ::GetCurrentProcessId();
	#elif POSIX
		return ::getpid();
	#else
		#error "Undefined platform"
	#endif
	}

	bool changeCurrentDirectory(const std::string &dir)
	{
	#ifdef WIN32
	#ifdef UNICODE
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
		const std::wstring target = converter.from_bytes(dir);
	#else
		const std::string &target = dir;
	#endif
		return 0 != ::SetCurrentDirectory(target.c_str() );
	#elif POSIX
		return 0 == ::chdir(dir.c_str() );
	#else
		#error "Undefined platform"
	#endif
	}

	bool isProcessExists(const native_processid_type pid) noexcept
	{
	#ifdef WIN32
		HANDLE hProcess = ::OpenProcess(SYNCHRONIZE, false, pid);
		::CloseHandle(hProcess);
		return 0 != hProcess;
	#elif POSIX
		return 0 == ::kill(pid, 0);
	#else
		#error "Undefined platform"
	#endif
	}

	bool sendSignal(const native_processid_type pid, const int signal) noexcept
	{
	#ifdef WIN32
		EnumData ed = {pid, 0};

		::EnumWindows(EnumProc, reinterpret_cast<::LPARAM>(&ed) );

		if (0 == ed.hWnd) {
			return false;
		}

		return 0 != ::PostMessage(ed.hWnd, signal, 0, 0);
	#elif POSIX
		return 0 == ::kill(pid, signal);
	#else
		#error "Undefined platform"
	#endif
	}

	bool isDoneThread(const std::thread::native_handle_type handle) noexcept
	{
	#ifdef WIN32
		return WAIT_OBJECT_0 == ::WaitForSingleObject(handle, 0);
	#elif POSIX
		return 0 != ::pthread_kill(handle, 0);
	#else
		#error "Undefined platform"
	#endif
	}

	std::string getTempDir()
	{
	#ifdef WIN32
		std::array<::TCHAR, MAX_PATH + 1> buf;

		auto const len = ::GetTempPath(static_cast<::DWORD>(buf.size() ), buf.data() );

		#ifdef UNICODE
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
			return converter.to_bytes(buf.data() );
		#else
			return std::string(buf.cbegin(), buf.cbegin() + len);
		#endif
	#elif POSIX
		const char *buf = ::getenv("TMPDIR");

		if (nullptr == buf) {
			return std::string("/tmp/");
		}

		std::string str(buf);

		if (str.back() != '/') {
			str.push_back('/');
		}

		return str;
	#else
		#error "Undefined platform"
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

		if (INVALID_FILE_ATTRIBUTES == attrib) {
			return false;
		}

		return FILE_ATTRIBUTE_DIRECTORY != (attrib & FILE_ATTRIBUTE_DIRECTORY);
	#elif POSIX
		struct ::stat attrib;

		if (-1 == ::stat(fileName.c_str(), &attrib) ) {
			return false;
		}

		return S_ISREG(attrib.st_mode);
	#else
		#error "Undefined platform"
	#endif
	}

	bool getFileSizeAndTimeGmt(
		const std::string &filePath,
		size_t *fileSize,
		time_t *fileTime
	) {
	#ifdef WIN32
		#ifdef UNICODE
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			const std::wstring file_path = converter.from_bytes(filePath);
		#else
			const std::string &file_path = filePath;
		#endif

		const ::HANDLE hFile = ::CreateFile(
			file_path.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			0,
			nullptr
		);

		if (INVALID_HANDLE_VALUE == hFile) {
			return false;
		}

		if (::GetFileSizeEx(hFile, reinterpret_cast<::PLARGE_INTEGER>(fileSize) ) == false) {
			::CloseHandle(hFile);
			return false;
		}

		::FILETIME ftWrite;

		::BOOL result = ::GetFileTime(
			hFile,
			nullptr,
			nullptr,
			&ftWrite
		);

		::CloseHandle(hFile);

		if (false == result) {
			return false;
		}

		::SYSTEMTIME stUtc;

		::FileTimeToSystemTime(&ftWrite, &stUtc);

		std::tm tm_time {
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

		*fileTime = std::mktime(&tm_time);

		return true;
	#elif POSIX
		struct ::stat attrib {};

		if (-1 == ::stat(filePath.c_str(), &attrib) ) {
			return false;
		}

		*fileSize = static_cast<size_t>(attrib.st_size);

		std::tm clock {};

		::gmtime_r(&(attrib.st_mtime), &clock);

		*fileTime = std::mktime(&clock);

		return true;
	#else
		#error "Undefined platform"
	#endif
	}

	void filterSharedMemoryName(std::string &memName)
	{
	#ifdef WIN32
	#ifdef UNICODE
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
		std::wstring memory_name = converter.from_bytes(memName);
		const std::wstring file_ext = L".exe";
	#else
		std::string &memory_name = memName;
		const std::string file_ext = ".exe";
	#endif

		const size_t pos = memory_name.rfind(file_ext);

		if (pos == memory_name.length() - file_ext.length() ) {
			memory_name.erase(
				pos,
				memory_name.length()
			);
		}

		::TCHAR buf[MAX_PATH + 1] {};
		::GetFullPathName(memory_name.c_str(), MAX_PATH, buf, nullptr);

	#ifdef UNICODE
		memName = converter.to_bytes(buf);
	#else
		memName = buf;
	#endif

		for (size_t i = 1; i < memName.length(); ++i) {
			if ('/' == memName[i] || '\\' == memName[i]) {
				memName[i] = '-';
			}
		}
	#elif POSIX
		if (memName.front() != '/') {
			memName = '/' + memName;
		}

		for (size_t i = 1; i < memName.length(); ++i) {
			if ('/' == memName[i] || '\\' == memName[i]) {
				memName[i] = '-';
			}
		}
	#else
		#error "Undefined platform"
	#endif
	}
}
