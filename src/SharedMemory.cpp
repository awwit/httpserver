
#include "SharedMemory.h"

#ifdef WIN32
	#include <Windows.h>
#elif POSIX
	#include <unistd.h>
	#include <sys/mman.h>
	#include <fcntl.h>
#endif

#include <cstring>

namespace HttpServer
{
	SharedMemory::SharedMemory()
	{
	#ifdef WIN32
		this->shm_desc = nullptr;
	#elif POSIX
		this->shm_desc = -1;
	#else
		#error "Undefine platform"
	#endif
	}

	SharedMemory::~SharedMemory()
	{
		this->close();
	}

	bool SharedMemory::create(const std::string &memName, const size_t memSize)
	{
		this->close();

	#ifdef WIN32

		this->shm_desc = ::CreateFileMapping(
			INVALID_HANDLE_VALUE,
			nullptr,
			PAGE_READWRITE,
			0,
			memSize,
			memName.c_str()
		);

		if (nullptr == this->shm_desc)
		{
			return false;
		}

	#elif POSIX

		this->shm_desc = ::shm_open(memName.c_str(), O_CREAT | O_RDWR, 0666);

		if (-1 == this->shm_desc)
		{
			return false;
		}

		if (-1 == ::ftruncate64(this->shm_desc, memSize) )
		{
			this->destroy(memName);

			return false;
		}

	#else
		#error "Undefine platform"
	#endif

		this->shm_name = memName;

		return true;
	}

	bool SharedMemory::open(const std::string &memName)
	{
		this->close();

	#ifdef WIN32

		this->shm_desc = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, false, memName.c_str() );

		if (nullptr == this->shm_desc)
		{
			return false;
		}

	#elif POSIX

		this->shm_desc = ::shm_open(memName.c_str(), O_RDWR, 0666);

		if (-1 == this->shm_desc)
		{
			return false;
		}

	#else
		#error "Undefine platform"
	#endif

		this->shm_name = memName;

		return true;
	}

	bool SharedMemory::is_open() const
	{
	#ifdef WIN32
		return nullptr != this->shm_desc;
	#elif POSIX
		return -1 != this->shm_desc;
	#else
		#error "Undefine platform"
	#endif
	}

	bool SharedMemory::write(const void *src, const size_t size, const size_t offset) const
	{
	#ifdef WIN32

		void * const addr = ::MapViewOfFile(this->shm_desc, FILE_MAP_WRITE, 0, offset, size);

		if (nullptr == addr)
		{
			return false;
		}

		::CopyMemory(addr, src, size);

		::UnmapViewOfFile(addr);

	#elif POSIX

		void * const addr = ::mmap(0, size, PROT_WRITE, MAP_SHARED, this->shm_desc, offset);

		if (reinterpret_cast<void *>(-1) == addr)
		{
			return false;
		}

		::memcpy(addr, src, size);

		::munmap(addr, size);

	#else
		#error "Undefine platform"
	#endif

		return true;
	}

	bool SharedMemory::read(void *dest, const size_t size, const size_t offset) const
	{
	#ifdef WIN32

		void * const addr = ::MapViewOfFile(this->shm_desc, FILE_MAP_READ, 0, offset, size);

		if (nullptr == addr)
		{
			return false;
		}

		::CopyMemory(dest, addr, size);

		::UnmapViewOfFile(addr);

	#elif POSIX

		void * const addr = ::mmap(0, size, PROT_READ, MAP_SHARED, this->shm_desc, offset);

		if (reinterpret_cast<const void *>(-1) == addr)
		{
			return false;
		}

		::memcpy(dest, addr, size);

		::munmap(addr, size);

	#else
		#error "Undefine platform"
	#endif

		return true;
	}

	bool SharedMemory::close()
	{
		if (this->is_open() )
		{
		#ifdef WIN32
			::CloseHandle(this->shm_desc);
			this->shm_desc = nullptr;
		#elif POSIX
			::close(this->shm_desc);
			this->shm_desc = ~0;
		#else
			#error "Undefine platform"
		#endif

			this->shm_name.clear();

			return true;
		}

		return false;
	}

	bool SharedMemory::destroy()
	{
	#ifdef WIN32
		bool ret = ::CloseHandle(this->shm_desc);

		this->close();

		return ret;
	#elif POSIX
		const int ret = ::shm_unlink(this->shm_name.c_str() );

		this->close();

		return 0 == ret;
	#else
		#error "Undefine platform"
	#endif
	}

	bool SharedMemory::destroy(const std::string &memName)
	{
	#ifdef WIN32
		return ::CloseHandle(this->shm_desc);
	#elif POSIX
		return 0 == ::shm_unlink(memName.c_str() );
	#else
		#error "Undefine platform"
	#endif
	}
};