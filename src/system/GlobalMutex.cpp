
#include "GlobalMutex.h"

#ifdef WIN32
	#include <Windows.h>

	#ifdef UNICODE
		#include <codecvt>
	#endif
#elif POSIX
	#include <fcntl.h>
#endif

namespace System
{
	GlobalMutex::GlobalMutex() noexcept
		: mtx_desc(nullptr)
	{

	}

	GlobalMutex::~GlobalMutex() noexcept
	{
		this->close();
	}

	bool GlobalMutex::create(const std::string &mutexName)
	{
		this->close();

	#ifdef WIN32

		this->mtx_name = "mtx-" + mutexName;

	#ifdef UNICODE
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
		const std::wstring mutex_name = converter.from_bytes(this->mtx_name);
	#else
		const std::string &mutex_name = this->mtx_name;
	#endif

		this->mtx_desc = ::CreateMutex(
			nullptr,
			false,
			mutex_name.c_str()
		);

		if (nullptr == this->mtx_desc) {
			return false;
		}
	#elif POSIX
		::sem_t *sem = ::sem_open(
			mutexName.c_str(),
			O_CREAT,
			0666,
			1
		);

		if (SEM_FAILED == sem) {
			return false;
		}

		this->mtx_desc = sem;
		this->mtx_name = mutexName;
	#else
		#error "Undefined platform"
	#endif

		return true;
	}

	bool GlobalMutex::destory(const std::string &mutexName)
	{
	#ifdef WIN32
		const std::string mtx_name = "mtx-" + mutexName;

	#ifdef UNICODE
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
		const std::wstring mutex_name = converter.from_bytes(mtx_name);
	#else
		const std::string &mutex_name = mtx_name;
	#endif

		const ::HANDLE hMutex = ::OpenMutex(
			DELETE,
			true,
			mutex_name.c_str()
		);

		return 0 != ::CloseHandle(hMutex);
	#elif POSIX
		return 0 == ::sem_unlink(mutexName.c_str() );
	#else
		#error "Undefined platform"
	#endif
	}

	bool GlobalMutex::destory()
	{
	#ifdef WIN32
	#ifdef UNICODE
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
		const std::wstring mutex_name = converter.from_bytes(this->mtx_name);
	#else
		const std::string &mutex_name = this->mtx_name;
	#endif

		const ::HANDLE hMutex = ::OpenMutex(
			DELETE,
			true,
			mutex_name.c_str()
		);

		const bool ret = (
			0 != ::CloseHandle(hMutex)
		);

		this->close();

		return ret;
	#elif POSIX
		const int ret = ::sem_unlink(
			this->mtx_name.c_str()
		);

		this->close();

		return 0 == ret;
	#else
		#error "Undefined platform"
	#endif
	}

	bool GlobalMutex::open(const std::string &mutexName)
	{
		this->close();

	#ifdef WIN32

		this->mtx_name = "mtx-" + mutexName;

	#ifdef UNICODE
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
		const std::wstring mutex_name = converter.from_bytes(this->mtx_name);
	#else
		const std::string &mutex_name = this->mtx_name;
	#endif

		this->mtx_desc = ::OpenMutex(
			SYNCHRONIZE,
			false,
			mutex_name.c_str()
		);

		if (nullptr == this->mtx_desc) {
			return false;
		}
	#elif POSIX
		::sem_t *sem = ::sem_open(
			mutexName.c_str(),
			0
		);

		if (SEM_FAILED == sem) {
			return false;
		}

		this->mtx_desc = sem;
	#else
		#error "Undefined platform"
	#endif

		this->mtx_name = mutexName;

		return true;
	}

	bool GlobalMutex::close() noexcept
	{
		if (this->mtx_desc)
		{
		#ifdef WIN32
			::CloseHandle(this->mtx_desc);
			this->mtx_desc = nullptr;
		#elif POSIX
			::sem_close(this->mtx_desc);
			this->mtx_desc = nullptr;
		#else
			#error "Undefined platform"
		#endif

			this->mtx_name.clear();

			return true;
		}

		return false;
	}

	bool GlobalMutex::is_open() const noexcept
	{
	#ifdef WIN32
		return nullptr != this->mtx_desc;
	#elif POSIX
		return nullptr != this->mtx_desc;
	#else
		#error "Undefined platform"
	#endif
	}

	bool GlobalMutex::lock() const noexcept
	{
	#ifdef WIN32
		return WAIT_OBJECT_0 == ::WaitForSingleObject(
			this->mtx_desc,
			INFINITE
		);
	#elif POSIX
		return 0 == ::sem_wait(this->mtx_desc);
	#else
		#error "Undefined platform"
	#endif
	}

	bool GlobalMutex::try_lock() const noexcept
	{
	#ifdef WIN32
		return WAIT_OBJECT_0 == ::WaitForSingleObject(
			this->mtx_desc,
			0
		);
	#elif POSIX
		return 0 == ::sem_trywait(this->mtx_desc);
	#else
		#error "Undefined platform"
	#endif
	}

	bool GlobalMutex::unlock() const noexcept
	{
	#ifdef WIN32
		return 0 != ::ReleaseMutex(this->mtx_desc);
	#elif POSIX
		return 0 == ::sem_post(this->mtx_desc);
	#else
		#error "Undefined platform"
	#endif
	}
}
