
#include "ServerControls.h"

namespace HttpServer
{
	ServerControls::ServerControls()
		: eventProcessQueue(nullptr),
		  eventNotFullQueue(nullptr),
		  eventUpdateModule(nullptr)
	{

	}

	ServerControls::~ServerControls() {
		this->clear();
	}

	void ServerControls::clear()
	{
		if (this->eventNotFullQueue) {
			delete this->eventNotFullQueue;
			this->eventNotFullQueue = nullptr;
		}

		if (this->eventProcessQueue) {
			delete this->eventProcessQueue;
			this->eventProcessQueue = nullptr;
		}

		if (this->eventUpdateModule) {
			delete this->eventUpdateModule;
			this->eventUpdateModule = nullptr;
		}
	}

	void ServerControls::setProcess(const bool flag) {
		this->process_flag = flag;
	}

	void ServerControls::stopProcess()
	{
		this->process_flag = false;

		if (this->eventNotFullQueue) {
			this->eventNotFullQueue->notify();
		}

		this->setProcessQueue();
	}

	void ServerControls::setRestart(const bool flag) {
		this->restart_flag = flag;
	}

	void ServerControls::setUpdateModule() {
		if (this->eventUpdateModule) {
			this->eventUpdateModule->notify();
		}
	}

	void ServerControls::setProcessQueue() {
		if (this->eventProcessQueue) {
			this->eventProcessQueue->notify();
		}
	}
}
