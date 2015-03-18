
#include "Module.h"

#include <iostream>

namespace HttpServer
{
	Module::Module(): lib_handle(nullptr)
	{
		
	}

	Module::Module(const std::string &libPath): lib_handle(nullptr)
	{
		open(libPath);
	}

	Module::Module(const Module &module) : lib_handle(module.lib_handle)
	{
		
	}

	Module::Module(Module &&module) : lib_handle(module.lib_handle)
	{
		module.lib_handle = nullptr;
	}

	bool Module::open(const std::string &libPath)
	{
		if (is_open() )
		{
			close();
		}

	#ifdef WIN32
		lib_handle = ::LoadLibrary(libPath.c_str() );
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

	Module &Module::operator =(const Module &module)
	{
		if (*this != module)
		{
			close();

			lib_handle = module.lib_handle;
		}

		return *this;
	}

	Module &Module::operator =(Module &&module)
	{
		if (*this != module)
		{
			close();

			lib_handle = module.lib_handle;

			module.lib_handle = nullptr;
		}

		return *this;
	}
};