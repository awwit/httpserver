#pragma once

#include "../socket/Adapter.h"
#include "../system/Module.h"

#include "ServerControls.h"
#include "ServerSettings.h"
#include "SocketsQueue.h"

#include <gnutls/gnutls.h>

namespace HttpServer
{
	class Server
	{
	protected:
		ServerSettings settings;

		std::unordered_map<int, std::tuple<gnutls_certificate_credentials_t, gnutls_priority_t> > tls_data;
	//	std::unordered_map<SocketAdapter *, std::queue<std::tuple<uint32_t> > > http_streams;

		std::vector<System::Module> modules;
		std::vector<Socket::Socket> liseners;

		mutable std::atomic_size_t threads_working_count;
		System::CachePadding<std::atomic_size_t> padding_1;

	public:
		mutable ServerControls controls;

	protected:
		int cycleQueue(SocketsQueue &sockets);

		void threadRequestProc(
			Socket::Adapter &sock,
			SocketsQueue &sockets,
			Http2::IncStream *stream
		) const;

		void threadRequestCycle(
			SocketsQueue &sockets,
			Utils::Event &eventThreadCycle
		) const;

		bool tryBindPort(
			const int port,
			std::unordered_set<int> &ports
		);

		void initAppsPorts();

		bool init();
		int run();
		void clear();

		static System::native_processid_type getServerProcessId(const std::string &serverName);

		void updateModules();

		bool updateModule(
			System::Module &module,
			std::unordered_set<ServerApplicationSettings *> &applications,
			const size_t moduleIndex
		);

	private:
		static bool get_start_args(
			const int argc,
			const char *argv[],
			struct server_start_args *st
		);

	public:
		Server() = default;

		void stop();
		void restart();
		void update();

		int command_help(const int argc, const char *argv[]) const;
		int command_start(const int argc, const char *argv[]);
		int command_restart(const int argc, const char *argv[]) const;
		int command_terminate(const int argc, const char *argv[]) const;
		int command_update_module(const int argc, const char *argv[]) const;
	};
}
