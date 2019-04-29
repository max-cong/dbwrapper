/*
 * Copyright (c) 2016-20019 Max Cong <savagecm@qq.com>
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
#include "loop/loop.hpp"
#include "util/nonCopyable.hpp"

namespace timer
{

class timer : public nonCopyable, public std::enable_shared_from_this<timer>
{
public:
	/** @brief callback fuction */
	typedef std::function<void()> Handler;
	typedef std::function<int(void *, int)> CBHandler;
	typedef std::shared_ptr<timer> ptr_p;

	timer() = delete;
	explicit timer(std::shared_ptr<loop::loop> loop) : _loop(loop),
													   _interval(0),
													   _round(1),
													   _curRound(0),
													   _handler(NULL),
													   _tid(0),
													   _isRunning(false)

	{
	}

	void setTid(unsigned long tid)
	{
		_tid = tid;
	}
	unsigned long getTid()
	{
		return _tid;
	}
	bool startRounds(uint32_t interval, uint64_t round, timer::timer::Handler const &handler)
	{
		if (_event_sptr)
		{
			__LOG(debug, "_event_sptr is valid, the timer is running, stop first then start");
			stop();
			//return false;
		}
		if (_loop.expired())
		{
			__LOG(error, "loop is invalid!");
			return false;
		}

		auto _event_base = (_loop.lock())->ev();
		if (!_event_base)
		{
			__LOG(warn, "event base is not valid!");
			return false;
		}

		_event_sptr.reset(event_new(_event_base, -1, (round > 1) ? EV_PERSIST : 0, eventHandler, this), [](event *innerEvent) {
			if (NULL != innerEvent)
			{
				event_free(innerEvent);
				innerEvent = NULL;
			}
		});
		if (!_event_sptr)
		{
			__LOG(error, "event is invalid");
			return false;
		}

		struct timeval tv = {};
		evutil_timerclear(&tv);
		tv.tv_sec = interval / 1000;
		tv.tv_usec = interval % 1000 * 1000;

		if (0 != event_add(_event_sptr.get(), &tv))
		{
			__LOG(error, "event add return fail");
			return false;
		}

		_interval = interval;
		_round = round;
		_curRound = 0;
		_handler = handler;
		setIsRunning(true);
		return true;
	}
	bool startOnce(uint32_t interval, timer::timer::Handler const &handler)
	{
		return startRounds(interval, 1, handler);
	}
	// note: this is not true forever
	bool startForever(uint32_t interval, timer::timer::Handler const &handler)
	{
		return startRounds(interval, uint32_t(-1), handler);
	}
	bool startAfter(
		uint32_t after,
		uint32_t intval,
		uint64_t rund,
		timer::timer::Handler const &handler)
	{
		auto self_wptr = getThisWptr();
		return startOnce(after, [intval, rund, handler, self_wptr]() {
			if (!self_wptr.expired())
			{
				self_wptr.lock()->startRounds(intval, rund, handler);
			}
			else
			{
				__LOG(warn, "timer: timer weak_ptr is exipred");
			}
		});
	}

	/** get timer interval */
	inline uint32_t interval() const
	{
		return _interval;
	}

	/** get timer rounds */
	inline uint64_t round() const
	{
		return _round;
	}

	bool getIsRunning()
	{
		return _isRunning && (_round > _curRound);
	}
	void setIsRunning(bool _running)
	{
		_isRunning = _running;
	}
	void stop()
	{
		if (!_event_sptr)
		{
			_event_sptr.reset();
			_curRound = 0;
			_round = 0;
		}
		setIsRunning(false);
	}
	std::shared_ptr<loop::loop> getLoop()
	{
		return _loop.lock();
	}
	std::weak_ptr<timer> getThisWptr()
	{

		std::weak_ptr<timer> timer_wptr(shared_from_this());

		return timer_wptr;
	}

private:
	// note: we expect we will not face the scanrio:
	// this handler is called but the timer obj is distroyed.
	// when the timer dis-truct, should stop all the handler
	static void eventHandler(evutil_socket_t fd, short events, void *ctx)
	{
		timer *_timer = (timer *)ctx;
		_timer->_curRound++;
		if (_timer->_curRound >= _timer->_round)
		{
			_timer->stop();
		}
		_timer->_handler();
	}

	std::weak_ptr<loop::loop> _loop;
	std::shared_ptr<event> _event_sptr;

	uint32_t _interval; //ms
	uint64_t _round;
	uint64_t _curRound;
	timer::timer::Handler _handler;
	unsigned long _tid;
	bool _isRunning;
};

} // namespace timer
