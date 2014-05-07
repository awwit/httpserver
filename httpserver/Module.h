#pragma once

#ifdef WIN32
	#include <windows.h>
#elif POSIX
	#include <dlfcn.h>
#else
	#error "Undefine platform"
#endif

#include <string>

namespace HttpServer
{
	class Module
	{
	protected:
	#ifdef WIN32
		HMODULE lib_handle;
	#elif POSIX
		void *lib_handle;
	#else
		#error "Undefine platform"
	#endif

	public:
		Module(): lib_handle(nullptr)
		{
			
		}

		Module(const std::string &libPath): lib_handle(nullptr)
		{
			open(libPath);
		}

		~Module() = default;

		inline bool is_open() const
		{
			return nullptr != lib_handle;
		}

		inline bool open(const std::string &libPath)
		{
		#ifdef WIN32
			lib_handle = LoadLibrary(libPath.c_str() );
		#elif POSIX
			lib_handle = dlopen(libPath.c_str(), RTLD_NOW);
		#else
			#error "Undefine platform"
		#endif

			return (nullptr != lib_handle);
		}

		inline void close()
		{
			if (lib_handle)
			{
			#ifdef WIN32
				FreeLibrary(lib_handle);
			#elif POSIX
				dlclose(lib_handle);
			#else
				#error "Undefine platform"
			#endif

				lib_handle = nullptr;
			}
		}

		inline bool find(const std::string &symbolName, void **addr) const
		{
			if (lib_handle)
			{
			#ifdef WIN32
				*addr = GetProcAddress(lib_handle, symbolName.c_str() );

				return nullptr != *addr;
			#elif POSIX
				dlerror();

				*addr = dlsym(lib_handle, symbolName.c_str() );

				char *error = dlerror();

				return nullptr == error;
			#else
				#error "Undefine platform"
			#endif
			}

			return false;
		}

		inline bool find(const char *symbolName, void **addr) const
		{
			if (lib_handle)
			{
			#ifdef WIN32
				*addr = GetProcAddress(lib_handle, symbolName);

				return nullptr != *addr;
			#elif POSIX
				*addr = dlsym(lib_handle, symbolName);

				char *error = dlerror();

				return nullptr == error;
			#else
				#error "Undefine platform"
			#endif
			}

			return false;
		}

		inline bool operator ==(const Module &module) const
		{
			return lib_handle == module.lib_handle;
		}
	};
};