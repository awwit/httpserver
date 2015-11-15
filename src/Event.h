#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>

namespace HttpServer
{
	class Event
	{
	private:
		std::mutex mtx;
		std::condition_variable cv;
		std::atomic<bool> signaled;
		bool manualy;

	public:
		Event(const bool _signaled = false, const bool _manualy = false);
		~Event() = default;

	public:
		void wait();
		bool wait_for(const std::chrono::milliseconds &ms);
		bool wait_until(const std::chrono::high_resolution_clock::time_point &tp);

		void notify();
		void notify(const size_t threadsCount);

		void reset();

		bool notifed() const;
	};
};