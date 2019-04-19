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
#include "loop/loop.hpp"
namespace timer
{

class timer
{
public:
	/** @brief callback fuction */
	typedef std::function<void()> Handler;
	typedef std::function<int(void *, int)> CBHandler;
	typedef std::shared_ptr<timer> ptr_p;

public:
	timer() = delete;
	timer(std::shared_ptr<loop::loop> loop) : _loop(loop),
										_event(NULL),
										_interval(0),
										_round(1),
										_curRound(0),
										_handler(NULL),

										_tid(0),
										_isRunning(false)

	{
	}
	virtual ~timer() { stop(); }
	void setTid(unsigned long tid)
	{
		_tid = tid;
	}
	unsigned long getTid()
	{
		return _tid;
	}
	bool startRounds(uint32_t interval, uint64_t round, timer::timer::Handler handler)
	{
		if (NULL != _event)
		{
			__LOG(error, "_event is not NULL, the timer is running, please stop first then start");
			return false;
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

		_event = event_new((_loop.lock())->ev(), -1, (round > 1) ? EV_PERSIST : 0, eventHandler, this);
		if (NULL == _event)
		{
			__LOG(error, "event is invalid");
			return false;
		}

		struct timeval tv = {};
		evutil_timerclear(&tv);
		tv.tv_sec = interval / 1000;
		tv.tv_usec = interval % 1000 * 1000;

		if (0 != event_add(_event, &tv))
		{
			__LOG(error, "event add return fail");
			//reset();
			return false;
		}

		_interval = interval;
		_round = round;
		_curRound = 0;
		_handler = handler;
		setIsRunning(true);
		return true;
	}
	bool startOnce(uint32_t interval, timer::timer::Handler handler)
	{
		return startRounds(interval, 1, handler);
	}
	// note: this is not true forever
	bool startForever(uint32_t interval, timer::timer::Handler handler)
	{
		return startRounds(interval, uint32_t(-1), handler);
	}
	bool startAfter(
		uint32_t after,
		uint32_t interval,
		uint64_t round,
		timer::timer::Handler handler)
	{
		return startOnce(after, [=]() {
			startRounds(interval, round, handler);
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
		if (NULL != _event)
		{
			event_free(_event);
			_event = NULL;
			_curRound = 0;
			_round = 0;
		}
		setIsRunning(false);
	}
	std::shared_ptr<loop::loop> get_loop()
	{
		return _loop.lock();
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

private:
	std::weak_ptr<loop::loop> _loop;
	struct event *_event;
	uint32_t _interval; //ms
	uint64_t _round;
	uint64_t _curRound;
	timer::timer::Handler _handler;
	unsigned long _tid;
	bool _isRunning;
};

} // namespace timer
