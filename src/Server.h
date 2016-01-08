#pragma once

#include "SocketList.h"
#include "DataVariantAbstract.h"
#include "ServerApplicationsTree.h"
#include "ServerApplicationDefaultSettings.h"
#include "Module.h"
#include "Event.h"

#include <cstddef>
#include <memory>
#include <vector>
#include <queue>
#include <csignal>
#include <atomic>

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
		static const int CONNECTION_CLOSED = 0;
		static const int CONNECTION_KEEP_ALIVE = 1;
		static const int CONNECTION_UPGRADE = 2;

	protected:
		int cycleQueue(std::queue<Socket> &sockets);

		int threadRequestProc(Socket clientSocket) const;

		static bool getRequest(Socket clientSocket, std::vector<char> &buf, std::string &str_buf, struct request_parameters &rp);

		int getRequestHeaders(std::string &str_buf, struct request_parameters &rp) const;

		static void runApplication(Socket clientSocket, const ServerApplicationSettings &appSets, struct request_parameters &rp);

		int getRequestData(Socket clientSocket, std::string &str_buf, const ServerApplicationSettings &appSets, struct request_parameters &rp) const;

		const ServerApplicationSettings *getApplicationSettings(const struct request_parameters &rp) const;

		static void getConnectionParams(struct request_parameters &rp);

		void xSendfile(Socket clientSocket, struct request_parameters &rp) const;

		static bool isConnectionKeepAlive(const struct request_parameters &rp);
		static bool isConnectionUpgrade(const struct request_parameters &rp);

		void threadRequestCycle(std::queue<Socket> &sockets) const;

		std::string getMimeTypeByFileName(const std::string &fileName) const;

		std::vector<std::tuple<size_t, size_t> > getRanges(
			const std::string &rangeHeader,
			const size_t posSymEqual,
			const size_t fileSize,
			std::string &resultRangeHeader,
			size_t &contentLength
		) const;

		int transferFilePart(
			const Socket &clientSocket,
			const std::chrono::milliseconds &timeout,
			const std::string &fileName,
			const time_t fileTime,
			const size_t fileSize,
			const std::string &rangeHeader,
			const std::string &connectionHeader,
			const std::string &dateHeader,
			const bool headersOnly
		) const;

		int transferFile(
			const Socket &clientSocket,
			const std::string &fileName,
			const std::string &connectionHeader,
			const bool headersOnly,
			struct request_parameters &rp
		) const;

		static bool parseIncomingVars(std::unordered_multimap<std::string, std::string> &params, const std::string &uriParams);
		static void sendStatus(const Socket &clientSocket, const std::chrono::milliseconds &timeout, const size_t statusCode);

		bool init();
		int run();
		void clear();

		System::native_processid_type getPidFromFile() const;

		void updateModules();
		bool updateModule(Module &module, std::unordered_set<ServerApplicationSettings *> &applications, const size_t moduleIndex);

	private:
		void addDataVariant(DataVariantAbstract *dataVariant);

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