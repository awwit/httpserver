#include "Event.h"

namespace HttpServer
{
	Event::Event(const bool _signaled, const bool _manualy): signaled(_signaled), manualy(_manualy)
	{
		
	}

	void Event::wait()
	{
		std::unique_lock<std::mutex> lck(mtx);

		while (false == signaled)
		{
			cv.wait(lck);
		}

		if (false == manualy)
		{
			signaled = false;
		}
	}

	bool Event::wait_for(const std::chrono::milliseconds &ms)
	{
		std::unique_lock<std::mutex> lck(mtx);

		auto const status = cv.wait_for(lck, ms);

		if (false == manualy)
		{
			signaled = false;
		}

		return std::cv_status::timeout == status;
	}

	bool Event::wait_until(const std::chrono::high_resolution_clock::time_point &tp)
	{
		std::unique_lock<std::mutex> lck(mtx);

		auto const status = cv.wait_until(lck, tp);

		if (false == manualy)
		{
			signaled = false;
		}

		return std::cv_status::timeout == status;
	}

	void Event::notify()
	{
	//	std::unique_lock<std::mutex> lck(mtx);
		signaled = true;
		cv.notify_all();
	}

	void Event::notify(const size_t threadsCount)
	{
		if (threadsCount)
		{
		//	std::unique_lock<std::mutex> lck(mtx);

			signaled = true;

			for (size_t i = 0; i < threadsCount; ++i)
			{
				cv.notify_one();
			}
		}
	}

	void Event::reset()
	{
	//	std::unique_lock<std::mutex> lck(mtx);

		signaled = false;
	}

	bool Event::notifed()
	{
	//	std::unique_lock<std::mutex> lck(mtx);

		return signaled;
	}
};