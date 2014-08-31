#pragma once

#include <mutex>
#include <condition_variable>

namespace HttpServer
{
	class Event
	{
	private:
		std::mutex mtx;
		std::condition_variable cv;
		bool signaled;
		bool manualy;

	public:
		Event(const bool _signaled = false, const bool _manualy = false);
		~Event() = default;

	public:
		void wait();
		void notify();
		void reset();

		bool notifed();
	};
};