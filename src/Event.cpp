
#include "Event.h"

namespace HttpServer
{
	Event::Event(const bool _signaled, const bool _manually): signaled(_signaled), manually(_manually)
	{
		
	}

	void Event::wait()
	{
		std::unique_lock<std::mutex> lck(mtx);

		while (false == signaled)
		{
			cv.wait(lck);
		}

		if (false == manually)
		{
			signaled = false;
		}
	}

	bool Event::wait_for(const std::chrono::milliseconds &ms)
	{
		std::unique_lock<std::mutex> lck(mtx);

		auto const status = cv.wait_for(lck, ms);

		if (false == manually)
		{
			signaled = false;
		}

		return std::cv_status::timeout == status;
	}

	bool Event::wait_until(const std::chrono::high_resolution_clock::time_point &tp)
	{
		std::unique_lock<std::mutex> lck(mtx);

		auto const status = cv.wait_until(lck, tp);

		if (false == manually)
		{
			signaled = false;
		}

		return std::cv_status::timeout == status;
	}

	void Event::notify()
	{
		signaled = true;
		cv.notify_all();
	}

	void Event::notify(const size_t threadsCount)
	{
		if (threadsCount)
		{
			signaled = true;

			for (size_t i = 0; i < threadsCount; ++i)
			{
				cv.notify_one();
			}
		}
	}

	void Event::reset()
	{
		signaled = false;
	}

	bool Event::notifed() const
	{
		return signaled;
	}
};