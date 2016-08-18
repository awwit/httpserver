
#include "Event.h"

namespace HttpServer
{
	Event::Event(const bool _signaled, const bool _manually): signaled(_signaled), manually(_manually)
	{
		
	}

	void Event::wait()
	{
		if (false == this->signaled.load() )
		{
			std::unique_lock<std::mutex> lck(this->mtx);

			do
			{
				this->cv.wait(lck);
			}
			while (false == this->signaled.load() );
		}

		if (false == this->manually)
		{
			this->signaled.store(false);
		}
	}

	bool Event::wait_for(const std::chrono::milliseconds &ms)
	{
		bool is_timeout = false;

		if (false == this->signaled.load() )
		{
			std::unique_lock<std::mutex> lck(this->mtx);

			is_timeout = false == this->cv.wait_for(lck, ms, [this] { return this->notifed(); } );
		}

		if (false == this->manually)
		{
			this->signaled.store(false);
		}

		return is_timeout;
	}

	bool Event::wait_until(const std::chrono::high_resolution_clock::time_point &tp)
	{
		bool is_timeout = false;

		if (false == this->signaled.load() )
		{
			std::unique_lock<std::mutex> lck(this->mtx);

			do
			{
				if (std::cv_status::timeout == this->cv.wait_until(lck, tp) )
				{
					is_timeout = true;
					break;
				}
			}
			while (false == this->signaled.load() );
		}

		if (false == this->manually)
		{
			this->signaled.store(false);
		}

		return is_timeout;
	}

	void Event::notify()
	{
		this->signaled.store(true);
		this->cv.notify_all();
	}

	void Event::notify(const size_t threadsCount)
	{
		this->signaled.store(true);

		for (size_t i = 0; i < threadsCount; ++i)
		{
			this->cv.notify_one();
		}
	}

	void Event::reset()
	{
		this->signaled.store(false);
	}

	bool Event::notifed() const
	{
		return this->signaled.load();
	}
};