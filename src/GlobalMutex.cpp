
#include "GlobalMutex.h"

#ifdef WIN32
	#include <Windows.h>
#elif POSIX
	#include <fcntl.h>
#endif

namespace HttpServer
{
	GlobalMutex::GlobalMutex() : mtx_desc(nullptr)
	{

	}

	GlobalMutex::~GlobalMutex()
	{
		this->close();
	}

	bool GlobalMutex::create(const std::string &mutexName)
	{
		this->close();

	#ifdef WIN32
		this->mtx_desc = ::CreateMutex(nullptr, false, mutexName.c_str() );

		if (nullptr == this->mtx_desc)
		{
			return false;
		}
	#elif POSIX
		::sem_t *sem = ::sem_open(mutexName.c_str(), O_CREAT, 0666, 1);

		if (SEM_FAILED == sem)
		{
			return false;
		}

		this->mtx_desc = sem;
	#else
		#error "Undefine platform"
	#endif

		this->mtx_name = mutexName;

		return true;
	}

	bool GlobalMutex::destory(const std::string &mutexName)
	{
	#ifdef WIN32
		return true;
	#elif POSIX
		return 0 == ::sem_unlink(mutexName.c_str() );
	#else
		#error "Undefine platform"
	#endif
	}

	bool GlobalMutex::destory()
	{
	#ifdef WIN32
		const bool ret = ::CloseHandle(this->mtx_desc);

		this->close();

		return ret;
	#elif POSIX
		 const int ret = ::sem_unlink(this->mtx_name.c_str() );

		 this->close();

		 return 0 == ret;
	#else
		#error "Undefine platform"
	#endif
	}

	bool GlobalMutex::open(const std::string &mutexName)
	{
		this->close();

	#ifdef WIN32
		this->mtx_desc = ::OpenMutex(SYNCHRONIZE, false, mutexName.c_str() );

		if (nullptr == this->mtx_desc)
		{
			return false;
		}
	#elif POSIX
		::sem_t *sem = ::sem_open(mutexName.c_str(), 0);

		if (SEM_FAILED == sem)
		{
			return false;
		}

		this->mtx_desc = sem;
	#else
		#error "Undefine platform"
	#endif

		this->mtx_name = mutexName;

		return true;
	}

	bool GlobalMutex::close()
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
			#error "Undefine platform"
		#endif

			this->mtx_name.clear();

			return true;
		}

		return false;
	}

	bool GlobalMutex::is_open() const
	{
	#ifdef WIN32
		return nullptr != this->mtx_desc;
	#elif POSIX
		return nullptr != this->mtx_desc;
	#else
		#error "Undefine platform"
	#endif
	}

	bool GlobalMutex::lock() const
	{
	#ifdef WIN32
		return WAIT_OBJECT_0 == ::WaitForSingleObject(this->mtx_desc, INFINITE);
	#elif POSIX
		return 0 == ::sem_wait(this->mtx_desc);
	#else
		#error "Undefine platform"
	#endif
	}

	bool GlobalMutex::try_lock() const
	{
	#ifdef WIN32
		return WAIT_OBJECT_0 == ::WaitForSingleObject(this->mtx_desc, 0);
	#elif POSIX
		return 0 == ::sem_trywait(this->mtx_desc);
	#else
		#error "Undefine platform"
	#endif
	}

	bool GlobalMutex::unlock() const
	{
	#ifdef WIN32
		return ::ReleaseMutex(this->mtx_desc);
	#elif POSIX
		return 0 == ::sem_post(this->mtx_desc);
	#else
		#error "Undefine platform"
	#endif
	}
};