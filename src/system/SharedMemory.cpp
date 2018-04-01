
#include "SharedMemory.h"

#ifdef WIN32
	#include <Windows.h>

	#ifdef UNICODE
		#include <codecvt>
	#endif
#elif POSIX
	#include <unistd.h>
	#include <sys/mman.h>
	#include <fcntl.h>
#endif

#include <cstring>

namespace System
{
	SharedMemory::SharedMemory() noexcept
	{
	#ifdef WIN32
		this->shm_desc = nullptr;
	#elif POSIX
		this->shm_desc = -1;
	#else
		#error "Undefined platform"
	#endif
	}

	SharedMemory::~SharedMemory() noexcept {
		this->close();
	}

	bool SharedMemory::create(
		const std::string &memName,
		const size_t memSize
	) {
		this->close();

	#ifdef WIN32

		this->shm_name = "shm-" + memName;

	#ifdef UNICODE
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
		const std::wstring memory_name = converter.from_bytes(this->shm_name);
	#else
		const std::string &memory_name = this->shm_name;
	#endif

		this->shm_desc = ::CreateFileMapping(
			INVALID_HANDLE_VALUE,
			nullptr,
			PAGE_READWRITE,
			0,
			static_cast<::DWORD>(memSize),
			memory_name.c_str()
		);

		if (nullptr == this->shm_desc) {
			return false;
		}

	#elif POSIX

		this->shm_desc = ::shm_open(
			memName.c_str(),
			O_CREAT | O_RDWR,
			0666
		);

		if (-1 == this->shm_desc) {
			return false;
		}

		if (::ftruncate64(this->shm_desc, long(memSize)) == -1) {
			this->destroy(memName);
			return false;
		}

		this->shm_name = memName;

	#else
		#error "Undefined platform"
	#endif

		return true;
	}

	bool SharedMemory::open(const std::string &memName)
	{
		this->close();

	#ifdef WIN32

		this->shm_name = "shm-" + memName;

	#ifdef UNICODE
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
		const std::wstring memory_name = converter.from_bytes(this->shm_name);
	#else
		const std::string &memory_name = this->shm_name;
	#endif

		this->shm_desc = ::OpenFileMapping(
			FILE_MAP_ALL_ACCESS,
			false,
			memory_name.c_str()
		);

		if (nullptr == this->shm_desc) {
			return false;
		}

		this->shm_name = memName;

	#elif POSIX

		this->shm_desc = ::shm_open(
			memName.c_str(),
			O_RDWR,
			0666
		);

		if (-1 == this->shm_desc) {
			return false;
		}

		this->shm_name = memName;

	#else
		#error "Undefined platform"
	#endif

		return true;
	}

	bool SharedMemory::is_open() const noexcept
	{
	#ifdef WIN32
		return nullptr != this->shm_desc;
	#elif POSIX
		return -1 != this->shm_desc;
	#else
		#error "Undefined platform"
	#endif
	}

	bool SharedMemory::write(
		const void *src,
		const size_t size,
		const size_t offset
	) const noexcept {
	#ifdef WIN32

		::ULARGE_INTEGER off;
		off.QuadPart = ::ULONGLONG(offset);

		void * const addr = ::MapViewOfFile(
			this->shm_desc,
			FILE_MAP_WRITE,
			off.HighPart,
			off.LowPart,
			size
		);

		if (nullptr == addr) {
			return false;
		}

		::CopyMemory(addr, src, size);

		::UnmapViewOfFile(addr);

	#elif POSIX

		void * const addr = ::mmap(
			nullptr,
			size,
			PROT_WRITE,
			MAP_SHARED,
			this->shm_desc,
			long(offset)
		);

		if (reinterpret_cast<void *>(-1) == addr) {
			return false;
		}

		std::memcpy(addr, src, size);

		::munmap(addr, size);

	#else
		#error "Undefined platform"
	#endif

		return true;
	}

	bool SharedMemory::read(
		void *dest,
		const size_t size,
		const size_t offset
	) const noexcept {
	#ifdef WIN32

		::ULARGE_INTEGER off;
		off.QuadPart = ::ULONGLONG(offset);

		void * const addr = ::MapViewOfFile(
			this->shm_desc,
			FILE_MAP_READ,
			off.HighPart,
			off.LowPart,
			size
		);

		if (nullptr == addr) {
			return false;
		}

		::CopyMemory(dest, addr, size);

		::UnmapViewOfFile(addr);

	#elif POSIX

		void * const addr = ::mmap(
			nullptr,
			size,
			PROT_READ,
			MAP_SHARED,
			this->shm_desc,
			long(offset)
		);

		if (reinterpret_cast<const void *>(-1) == addr) {
			return false;
		}

		std::memcpy(dest, addr, size);

		::munmap(addr, size);

	#else
		#error "Undefined platform"
	#endif

		return true;
	}

	bool SharedMemory::close() noexcept
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
			#error "Undefined platform"
		#endif

			this->shm_name.clear();

			return true;
		}

		return false;
	}

	bool SharedMemory::destroy()
	{
	#ifdef WIN32
	#ifdef UNICODE
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
		const std::wstring memory_name = converter.from_bytes(this->shm_name);
	#else
		const std::string &memory_name = this->shm_name;
	#endif

		const ::HANDLE hMemory = ::OpenFileMapping(
			DELETE,
			false,
			memory_name.c_str()
		);

		const bool ret = (
			0 != ::CloseHandle(hMemory)
		);

		this->close();

		return ret;
	#elif POSIX
		const int ret = ::shm_unlink(
			this->shm_name.c_str()
		);

		this->close();

		return 0 == ret;
	#else
		#error "Undefined platform"
	#endif
	}

	bool SharedMemory::destroy(const std::string &memName)
	{
	#ifdef WIN32
		const std::string shm_name = "shm-" + memName;

	#ifdef UNICODE
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
		const std::wstring memory_name = converter.from_bytes(shm_name);
	#else
		const std::string &memory_name = shm_name;
	#endif

		const ::HANDLE hMemory = ::OpenFileMapping(
			DELETE,
			false,
			memory_name.c_str()
		);

		return 0 != ::CloseHandle(hMemory);
	#elif POSIX
		return 0 == ::shm_unlink(memName.c_str() );
	#else
		#error "Undefined platform"
	#endif
	}
}
