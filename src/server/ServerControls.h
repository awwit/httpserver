#pragma once

#include "../utils/Event.h"
#include "../system/Cache.h"

#include <atomic>
#include <csignal>

namespace HttpServer
{
	class ServerControls
	{
	public:
		Utils::Event *eventProcessQueue;
		Utils::Event *eventNotFullQueue;
		Utils::Event *eventUpdateModule;

		System::CachePaddingSize<sizeof(void *) * 3> padding_1;

		// Флаг, означающий - активированы ли главные циклы сервера
		// (с помощью этого флага можно деактивировать циклы, чтобы завершить работу сервера)
		sig_atomic_t process_flag;
		sig_atomic_t restart_flag;

	public:
		ServerControls();
		~ServerControls();

		void clear();

		void setProcess(const bool flag = true);
		void stopProcess();
		void setRestart(const bool flag = true);
		void setUpdateModule();
		void setProcessQueue();
	};
}
