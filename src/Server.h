#pragma once

#include "SocketList.h"
#include "DataVariantAbstract.h"
#include "ServerApplicationsTree.h"
#include "ServerApplicationDefaultSettings.h"
#include "Module.h"
#include "Event.h"
#include "SocketAdapter.h"
#include "ServerStructuresArguments.h"

#include <cstddef>
#include <memory>
#include <vector>
#include <queue>
#include <csignal>
#include <atomic>

#include <gnutls/gnutls.h>

namespace HttpServer
{
	class Server
	{
	protected:
		std::unordered_map<std::string, DataVariantAbstract *> variants;

		std::unordered_map<std::string, std::string> settings;
		std::unordered_map<std::string, std::string> mimes_types;

		std::unordered_map<int, std::tuple<gnutls_certificate_credentials_t, gnutls_priority_t> > tls_data;

		std::vector<Module> modules;

		ServerApplicationsTree apps_tree;

		std::vector<Socket> server_sockets;

		Event *eventNotFullQueue;
		Event *eventProcessQueue;
		Event *eventUpdateModule;

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
		int cycleQueue(std::queue<std::tuple<Socket, struct sockaddr_in > > &sockets);

		int threadRequestProc(SocketAdapter &clientSocket, const struct sockaddr_in &clientAddr) const;

		static bool getRequest(const SocketAdapter &clientSocket, std::vector<char> &buf, std::string &str_buf, struct request_parameters &rp);

		int getRequestHeaders(std::string &str_buf, struct request_parameters &rp) const;

		static void runApplication(const SocketAdapter &clientSocket, const ServerApplicationSettings &appSets, struct request_parameters &rp);

		int getRequestData(const SocketAdapter &clientSocket, std::string &str_buf, const ServerApplicationSettings &appSets, struct request_parameters &rp) const;

		const ServerApplicationSettings *getApplicationSettings(const struct request_parameters &rp) const;

		static void getConnectionParams(struct request_parameters &rp);

		void xSendfile(const SocketAdapter &clientSocket, struct request_parameters &rp) const;

		static bool isConnectionKeepAlive(const struct request_parameters &rp);
		static bool isConnectionUpgrade(const struct request_parameters &rp);

		void threadRequestCycle(std::queue<std::tuple<Socket, struct sockaddr_in> > &sockets, Event &eventThreadCycle) const;

		std::string getMimeTypeByFileName(const std::string &fileName) const;

		std::vector<std::tuple<size_t, size_t> > getRanges(
			const std::string &rangeHeader,
			const size_t posSymEqual,
			const size_t fileSize,
			std::string &resultRangeHeader,
			size_t &contentLength
		) const;

		int transferFilePart(
			const SocketAdapter &clientSocket,
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
			const SocketAdapter &clientSocket,
			const std::string &fileName,
			const std::string &connectionHeader,
			const bool headersOnly,
			struct request_parameters &rp
		) const;

		static bool parseIncomingVars(std::unordered_multimap<std::string, std::string> &params, const std::string &uriParams);
		static void sendStatus(const SocketAdapter &clientSocket, const std::chrono::milliseconds &timeout, const size_t statusCode);

		static bool tlsInit(const ServerApplicationSettings &app, std::tuple<gnutls_certificate_credentials_t, gnutls_priority_t> &data);

		bool tryBindPort(const int port, std::unordered_set<int> &ports);
		void initAppsPorts();

		bool init();
		int run();
		void clear();

		static System::native_processid_type getServerProcessId(const std::string &serverName);

		void updateModules();
		bool updateModule(Module &module, std::unordered_set<ServerApplicationSettings *> &applications, const size_t moduleIndex);

	private:
		void addDataVariant(DataVariantAbstract *dataVariant);

		static bool get_start_args(const int argc, const char *argv[], struct server_start_args *st);

	public:
		Server();
		~Server() = default;

		void stopProcess();
		void unsetProcess();
		void setRestart();
		void setUpdateModule();
		void setProcessQueue();

		int command_help(const int argc, const char *argv[]) const;
		int command_start(const int argc, const char *argv[]);
		int command_restart(const int argc, const char *argv[]) const;
		int command_terminate(const int argc, const char *argv[]) const;
		int command_update_module(const int argc, const char *argv[]) const;
	};
};