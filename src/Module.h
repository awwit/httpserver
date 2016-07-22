#pragma once

#ifdef WIN32
	#include <wtypes.h>
	#include <WinDef.h>
#endif

#include <string>

namespace HttpServer
{
	class Module
	{
	protected:
	#ifdef WIN32
		::HMODULE lib_handle;
	#elif POSIX
		void *lib_handle;
	#else
		#error "Undefine platform"
	#endif

	public:
		Module();
		Module(const std::string &libPath);
		Module(const Module &obj);
		Module(Module &&obj);

		~Module() = default;

		bool is_open() const;

		bool open(const std::string &libPath);
		void close();

		bool find(const std::string &symbolName, void *(**addr)(void *) ) const;
		bool find(const char *symbolName, void *(**addr)(void *) ) const;

		bool operator ==(const Module &obj) const;
		bool operator !=(const Module &obj) const;

		Module &operator =(const Module &obj);
		Module &operator =(Module &&obj);
	};
};