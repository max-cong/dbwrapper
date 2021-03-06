/*
 * Copyright (c) 2016-20017 Max Cong <savagecm@qq.com>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <memory>
#include <map>
#include <mutex>
#include <thread>
#include <string>
#include <atomic>
#include <functional>
#include <thread>
#include <event2/event.h>
#include <event2/thread.h>
#include "logger/logger.hpp"
#include "gene/gene.hpp"

namespace loop
{
enum class loopStatus : std::uint32_t
{
	statusInit = 0,
	statusRunning,
	statusFinished,
	statisMax
};
static std::ostream &operator<<(std::ostream &os, loopStatus status)
{
	std::string statusString = ((status >= loopStatus::statusInit || status < loopStatus::statisMax) ? (const std::string[]){"statusInit", "statusRunning", "statusFinished"}[(int)status] : "UNDEFINED_STATUS");
	os << statusString;
	return os;
}
class loop : public gene::gene<void *>, public std::enable_shared_from_this<loop>
{
public:
	loop() : _status(loopStatus::statusInit)
	{
		__LOG(error, " in the loop constructor, status is :" << _status);
	}
	/** convert to event_base * pointer*/
	inline explicit operator event_base *() const
	{
		return _base_sptr.get();
	};

	/** @brief get event_base * pointer*/
	inline event_base *ev() const
	{
		return _base_sptr.get();
	}

	/** get status */
	loopStatus status()
	{
		return _status;
	}

	/**
	 * @brief start event loop
	 * @details if this is called in the current thread. it will block current thread until the end of time loop
	 * see onBeforeStart
	 */
	bool start(bool newThread = true)
	{
		evthread_use_pthreads();
		_base_sptr.reset(event_base_new(), [](event_base *innerBase) {
			if (NULL != innerBase)
			{
				if (CHECK_LOG_LEVEL(warn))
				{
					__LOG(warn, "[loop] event base is exiting!");
				}
				event_base_free(innerBase);
				innerBase = NULL;
			}
		});

		if (!_base_sptr)
		{
			return false;
		}

		if (_status != loopStatus::statusInit)
		{
			if (CHECK_LOG_LEVEL(error))
			{
				__LOG(error, " conneciton status is not Init state, status is :" << _status);
			}
			return false;
		}

		if (!onBeforeStart())
		{
			if (CHECK_LOG_LEVEL(error))
			{
				__LOG(error, "onBeforeStart return fail");
			}
			return false;
		}
		if (newThread)
		{
			// take care here
			//_loopThread = std::make_shared<std::thread>(std::bind(&loop::_run, shared_from_this()));
			_loopThread = std::make_shared<std::thread>(std::bind(&loop::_run, this));
		}
		else
		{
			_run();
		}
		return true;
	}

	/** if run with new thread, will stop the new thread
	 * waiting: if call the active callback before exit
	 */
	void stop(bool waiting = true)
	{

		if (loopStatus::statusFinished == _status)
		{
			if (CHECK_LOG_LEVEL(warn))
			{
				__LOG(warn, "now try to stop loop, but status is finished");
			}
			return;
		}

		if (CHECK_LOG_LEVEL(warn))
		{
			__LOG(warn, "now try to stop loop, event base is : " << (void *)_base_sptr.get());
		}
		waiting ? event_base_loopexit(_base_sptr.get(), NULL) : event_base_loopbreak(this->ev());
		onAfterStop();

		if (_loopThread) //&& loopStatus::statusFinished != _status)
		{
			_loopThread->join();
		}
		_loopThread = nullptr;
	}

protected:
	bool onBeforeStart()
	{
		return true;
	}
	void onBeforeLoop()
	{
		if (CHECK_LOG_LEVEL(debug))
		{
			__LOG(debug, "onBeforeLoop");
		}
	}
	void onAfterLoop()
	{
		if (CHECK_LOG_LEVEL(debug))
		{
			__LOG(debug, "onAfterLoop");
		}
	}
	void onAfterStop()
	{
		if (CHECK_LOG_LEVEL(debug))
		{
			__LOG(debug, "onAfterStop");
		}
	}
	static void eventHandler(evutil_socket_t fd, short events, void *ctx)
	{
		if (CHECK_LOG_LEVEL(debug))
		{
			__LOG(debug, "[Loop] I am running");
		}
	}

private:
	void _run()
	{
		_status = loopStatus::statusRunning;

		onBeforeLoop();
		if (CHECK_LOG_LEVEL(debug))
		{
			__LOG(debug, " start loop!! base event is : " << (void *)ev());
		}
		// add one timer to keep loop running
		auto event_ptr = event_new(this->ev(), -1, EV_PERSIST, loop::eventHandler, NULL);

		struct timeval tv = {};
		evutil_timerclear(&tv);
		uint32_t interval = 10000;
		tv.tv_sec = interval / 1000;
		tv.tv_usec = interval % 1000 * 1000;

		if (0 != event_add(event_ptr, &tv))
		{
			if (CHECK_LOG_LEVEL(error))
			{
				__LOG(error, "event add return fail");
			}
		}

		event_base_loop(this->ev(), 0);
		if (CHECK_LOG_LEVEL(warn))
		{
			__LOG(warn, " exit loop!!");
		}
		onAfterLoop();

		_status = loopStatus::statusFinished;
	}

	std::shared_ptr<event_base> _base_sptr;
	std::shared_ptr<std::thread> _loopThread;
	std::atomic<loopStatus> _status;
};
} // namespace loop
