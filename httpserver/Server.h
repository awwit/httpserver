#pragma once

#include <cstddef>

#include <memory>
#include <vector>
#include <queue>
#include <string>
#include <unordered_map>
#include <csignal>

#include "Socket.h"
#include "DataVariantAbstract.h"
#include "ServerApplicationsTree.h"
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

		Socket server_socket;

		Event *eventNotFullQueue;
		Event *eventProcessQueue;

		// Флаг, означающий - активированы ли главные циклы сервера
		// (с помощью этого флага можно деактивировать циклы, чтобы завершить работу сервера)
		sig_atomic_t process_flag;
		sig_atomic_t restart_flag;

	protected:
		int cycleQueue(std::queue<std::shared_ptr<Socket> > &);
		int threadRequestProc(Socket *);
		int transferFile(const Socket *, const std::string &, const std::unordered_map<std::string, std::string> &, const std::map<std::string, std::string> &, const std::string &) const;
		void parseIncomingVars(std::unordered_multimap<std::string, std::string> &, const std::string &, const size_t, const size_t) const;

		bool includeConfigFile(const std::string &, std::string &, const size_t);
		bool loadConfig();

		bool init();
		int run();
		void clear();

		System::native_processid_type getPidFromFile() const;

	private:
		void addDataVariant(DataVariantAbstract *);

	public:
		Server();
		~Server() = default;

		void stopProcess();

		inline void setRestart()
		{
			restart_flag = true;
		}

		int help() const;
		int start();
		int restart() const;
		int terminate() const;
	};
};
