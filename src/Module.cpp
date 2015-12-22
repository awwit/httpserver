
#include "Module.h"

#ifdef WIN32
	#ifdef UNICODE
		#include <codecvt>
	#endif
#endif

namespace HttpServer
{
	Module::Module(): lib_handle(nullptr)
	{
		
	}

	Module::Module(const std::string &libPath): lib_handle(nullptr)
	{
		open(libPath);
	}

	Module::Module(const Module &obj) : lib_handle(obj.lib_handle)
	{
		
	}

	Module::Module(Module &&obj) : lib_handle(obj.lib_handle)
	{
		obj.lib_handle = nullptr;
	}

	bool Module::open(const std::string &libPath)
	{
		if (is_open() )
		{
			close();
		}

	#ifdef WIN32
		const size_t pos_slash = libPath.rfind('\\');
		const size_t pos_slash_back = libPath.rfind('/');

		size_t pos = std::string::npos;

		if (pos_slash != std::string::npos && pos_slash > pos_slash_back)
		{
			pos = pos_slash;
		}
		else if (pos_slash_back != std::string::npos)
		{
			pos = pos_slash_back;
		}

		DLL_DIRECTORY_COOKIE cookie = nullptr;

		if (std::string::npos != pos)
		{
			std::wstring directory(libPath.cbegin(), libPath.cbegin() + pos + 1);

			cookie = ::AddDllDirectory(directory.data() );
		}

		#ifdef UNICODE
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			const std::wstring lib_path = converter.from_bytes(libPath);
		#else
			const std::string &lib_path = libPath;
		#endif
		
		lib_handle = ::LoadLibraryEx(lib_path.c_str(), 0, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

		if (cookie)
		{
			::RemoveDllDirectory(cookie);
		}
	#elif POSIX
		lib_handle = ::dlopen(libPath.c_str(), RTLD_NOW | RTLD_LOCAL);
	#else
		#error "Undefine platform"
	#endif

		if (nullptr == lib_handle)
		{
		#ifdef POSIX
            std::cout << ::dlerror() << std::endl;
		#endif
			return false;
		}

		return true;
	}

	void Module::close()
	{
		if (lib_handle)
		{
		#ifdef WIN32
			::FreeLibrary(lib_handle);
		#elif POSIX
			::dlclose(lib_handle);
		#else
			#error "Undefine platform"
		#endif

			lib_handle = nullptr;
		}
	}

	bool Module::find(const std::string &symbolName, void *(**addr)(void *) ) const
	{
		if (lib_handle)
		{
		#ifdef WIN32
			*addr = reinterpret_cast<void *(*)(void *)>(::GetProcAddress(lib_handle, symbolName.c_str() ) );

			return nullptr != *addr;
		#elif POSIX
			char *error = ::dlerror();

			*addr = reinterpret_cast<void *(*)(void *)>(::dlsym(lib_handle, symbolName.c_str() ) );

			error = ::dlerror();

			return nullptr == error;
		#else
			#error "Undefine platform"
		#endif
		}

		return false;
	}

	bool Module::find(const char *symbolName, void *(**addr)(void *) ) const
	{
		if (lib_handle)
		{
		#ifdef WIN32
			*addr = reinterpret_cast<void *(*)(void *)>(::GetProcAddress(lib_handle, symbolName) );

			return nullptr != *addr;
		#elif POSIX
			char *error = ::dlerror();

			*addr = reinterpret_cast<void *(*)(void *)>(::dlsym(lib_handle, symbolName) );

			error = ::dlerror();

			return nullptr == error;
		#else
			#error "Undefine platform"
		#endif
		}

		return false;
	}

	Module &Module::operator =(const Module &obj)
	{
		if (*this != obj)
		{
			close();

			lib_handle = obj.lib_handle;
		}

		return *this;
	}

	Module &Module::operator =(Module &&obj)
	{
		if (*this != obj)
		{
			close();

			lib_handle = obj.lib_handle;

			obj.lib_handle = nullptr;
		}

		return *this;
	}
};