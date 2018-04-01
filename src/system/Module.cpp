
#include "Module.h"

#ifdef WIN32
	#include <Windows.h>

	#ifdef UNICODE
		#include <codecvt>
	#endif
#elif POSIX
	#include <dlfcn.h>
	#include <iostream>
#endif

namespace System
{
	Module::Module() noexcept
		: lib_handle(nullptr)
	{
		
	}

	Module::Module(const std::string &libPath)
		: lib_handle(nullptr)
	{
		open(libPath);
	}

	Module::Module(const Module &obj) noexcept
		: lib_handle(obj.lib_handle)
	{
		
	}

	Module::Module(Module &&obj) noexcept
		: lib_handle(obj.lib_handle)
	{
		obj.lib_handle = nullptr;
	}

	bool Module::is_open() const noexcept {
		return nullptr != this->lib_handle;
	}

	bool Module::open(const std::string &libPath)
	{
		if (this->is_open() ) {
			this->close();
		}

	#ifdef WIN32
		const size_t pos_slash = libPath.rfind('\\');
		const size_t pos_slash_back = libPath.rfind('/');

		const size_t pos = (
			pos_slash != std::string::npos &&
			pos_slash > pos_slash_back
				? pos_slash
				: pos_slash_back != std::string::npos
					? pos_slash_back
					: std::string::npos
		);

		::DLL_DIRECTORY_COOKIE cookie = nullptr;

		if (std::string::npos != pos) {
			const std::wstring directory(
				libPath.cbegin(),
				libPath.cbegin() + pos + 1
			);

			cookie = ::AddDllDirectory(
				directory.data()
			);
		}

		#ifdef UNICODE
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
			const std::wstring lib_path = converter.from_bytes(libPath);
		#else
			const std::string &lib_path = libPath;
		#endif

		this->lib_handle = ::LoadLibraryEx(
			lib_path.c_str(),
			0,
			LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
		);

		if (cookie) {
			::RemoveDllDirectory(cookie);
		}
	#elif POSIX
		this->lib_handle = ::dlopen(
			libPath.c_str(),
			RTLD_NOW | RTLD_LOCAL
		);
	#else
		#error "Undefined platform"
	#endif

		if (nullptr == this->lib_handle)
		{
		#ifdef POSIX
			std::cout << ::dlerror() << std::endl;
		#endif
			return false;
		}

		return true;
	}

	void Module::close() noexcept
	{
		if (this->lib_handle)
		{
		#ifdef WIN32
			::FreeLibrary(this->lib_handle);
		#elif POSIX
			::dlclose(this->lib_handle);
		#else
			#error "Undefined platform"
		#endif

			this->lib_handle = nullptr;
		}
	}

	bool Module::find(
		const std::string &symbolName,
		void *(**addr)(void *)
	) const noexcept {
		if (lib_handle) {
		#ifdef WIN32
			*addr = reinterpret_cast<void *(*)(void *)>(
				::GetProcAddress(
					this->lib_handle,
					symbolName.c_str()
				)
			);

			return nullptr != *addr;
		#elif POSIX
			char *error = ::dlerror();

			*addr = reinterpret_cast<void *(*)(void *)>(
				::dlsym(
					this->lib_handle,
					symbolName.c_str()
				)
			);

			error = ::dlerror();

			return nullptr == error;
		#else
			#error "Undefined platform"
		#endif
		}

		return false;
	}

	bool Module::find(
		const char *symbolName,
		void *(**addr)(void *)
	) const noexcept {
		if (lib_handle)
		{
		#ifdef WIN32
			*addr = reinterpret_cast<void *(*)(void *)>(
				::GetProcAddress(
					this->lib_handle,
					symbolName
				)
			);

			return nullptr != *addr;
		#elif POSIX
			char *error = ::dlerror();

			*addr = reinterpret_cast<void *(*)(void *)>(
				::dlsym(
					this->lib_handle,
					symbolName
				)
			);

			error = ::dlerror();

			return nullptr == error;
		#else
			#error "Undefined platform"
		#endif
		}

		return false;
	}

	bool Module::operator ==(const Module &obj) const noexcept {
		return this->lib_handle == obj.lib_handle;
	}

	bool Module::operator !=(const Module &obj) const noexcept {
		return this->lib_handle != obj.lib_handle;
	}

	Module &Module::operator =(const Module &obj) noexcept
	{
		if (*this != obj) {
			this->close();
			this->lib_handle = obj.lib_handle;
		}

		return *this;
	}

	Module &Module::operator =(Module &&obj) noexcept
	{
		if (*this != obj) {
			this->close();
			this->lib_handle = obj.lib_handle;
			obj.lib_handle = nullptr;
		}

		return *this;
	}
}
