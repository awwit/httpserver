#pragma once

#include <cstddef>

#include <memory>

#include <vector>
#include <queue>
#include <string>
#include <unordered_map>
#include <csignal>
#include <atomic>

#include "SocketList.h"
#include "DataVariantAbstract.h"
#include "ServerApplicationsTree.h"
#include "ServerApplicationDefaultSettings.h"
#include "Module.h"
#include "Event.h"

namespace HttpServer
{
	class Server
	{
	protected:
		std::unordered_map<std::string, DataVariantAbstract *> variants;

		std::unordered_map<std::string, std::string> settings;
		std::unordered_map<std::string, std::string> mimes_types;

		std::vector<Module> modules;

		ServerApplicationsTree apps_tree;

		std::vector<Socket> server_sockets;

		Event *eventNotFullQueue;
		Event *eventProcessQueue;
		Event *eventUpdateModule;
        Event *eventThreadCycle;

        mutable std::atomic_size_t threads_working_count;
        mutable std::mutex sockets_queue_mtx;

		// Флаг, означающий - активированы ли главные циклы сервера
		// (с помощью этого флага можно деактивировать циклы, чтобы завершить работу сервера)
		sig_atomic_t process_flag;
		sig_atomic_t restart_flag;

	protected:
		int cycleQueue(std::queue<Socket> &);
		void sendStatus(const Socket &, const std::chrono::milliseconds &, const size_t) const;
        int threadRequestProc(Socket) const;
        void threadRequestCycle(std::queue<Socket> &) const;
		int transferFilePart(const Socket &, const std::chrono::milliseconds &, const std::string &, const time_t, const size_t, const std::string &, const std::string &, const std::string &, const bool) const;

		int transferFile(
			const Socket &,
			const std::chrono::milliseconds &,
			const std::string &,
			const std::unordered_map<std::string, std::string> &,
			const std::unordered_map<std::string, std::string> &,
			const std::string &,
			const bool
		) const;

		bool parseIncomingVars(std::unordered_multimap<std::string, std::string> &, const std::string &) const;

		bool init();
		int run();
		void clear();

		System::native_processid_type getPidFromFile() const;

		void updateModules();
			bool updateModule(Module &, std::unordered_set<ServerApplicationSettings *> &, const size_t);

	private:
		void addDataVariant(DataVariantAbstract *);

	public:
		Server();
		~Server() = default;

		void stopProcess();

		inline void unsetProcess()
		{
			this->process_flag = false;
		}

		inline void setRestart()
		{
			this->restart_flag = true;
		}

		inline void setUpdateModule()
		{
			if (this->eventUpdateModule)
			{
				this->eventUpdateModule->notify();
			}
		}

		inline void setProcessQueue()
		{
			if (this->eventProcessQueue)
			{
				this->eventProcessQueue->notify();
			}
		}

		int command_help(const int argc, const char *argv[]) const;
		int command_start(const int argc, const char *argv[]);
		int command_restart(const int argc, const char *argv[]) const;
		int command_terminate(const int argc, const char *argv[]) const;
		int command_update_module(const int argc, const char *argv[]) const;
	};
};