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

	void Event::notify()
	{
		std::unique_lock<std::mutex> lck(mtx);
		signaled = true;
		cv.notify_all();
	}

	void Event::reset()
	{
		std::unique_lock<std::mutex> lck(mtx);

		signaled = false;
	}

	bool Event::notifed()
	{
		std::unique_lock<std::mutex> lck(mtx);

		return signaled;
	}
};