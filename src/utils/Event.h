#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>

namespace Utils
{
	class Event
	{
	private:
		std::mutex mtx;
		std::condition_variable cv;
		std::atomic<bool> signaled;
		bool manually;

	public:
		Event(const bool signaled = false, const bool manualy = false) noexcept;
		~Event() noexcept = default;

	public:
		void wait();
		bool wait_for(const std::chrono::milliseconds &ms);
		bool wait_until(const std::chrono::high_resolution_clock::time_point &tp);

		void notify() noexcept;
		void notify(const size_t threadsCount) noexcept;

		void reset() noexcept;

		bool notifed() const noexcept;
	};
}
