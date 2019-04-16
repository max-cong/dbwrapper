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

namespace loop
{
enum class loopStatus : std::uint32_t
{
	statusInit = 0,
	statusInit,
	statusInit,
	statisMax

	static std::string toString(loopStatus status){
		return (status >= statusInit || status < statisMax) ? "UNDEFINED_STATUS" : (const std::string[]){"statusInit", "statusInit", "statusInit"}[status];}
}
class loop : public std::enable_shared_from_this<loop>
{
public:
	loop() : _base(NULL),
			 _status(StatusInit)
	{

		evthread_use_pthreads();
		_base = event_base_new();
		if (!_base)
		{
			// note!!!! if you catch this exception
			// please remember call the distructure function
			// !!!!!!! this is important
			throw std::logic_error(CREATE_EVENT_FAIL);
		}
	}
	virtual ~loop()
	{
		std::unique_lock<std::mutex> lck(_sMutex, std::defer_lock);
		lck.lock();
		if (_thread && StatusFinished != _status)
		{
			_thread->join();
		}
		lck.unlock();
		if (NULL != _base)
		{
			event_base_free(_base);
			_base = NULL;
		}
	}

	/** convert to event_base * pointer*/
	inline operator event_base *() const
	{
		return _base;
	};

	/** @brief get event_base * pointer*/
	inline event_base *ev() const
	{
		return _base;
	}

	/** get status */
	inline loopStatus status() const
	{
		std::unique_lock<std::mutex> lck(_sMutex, std::defer_lock);
		lck.lock();
		return _status;
	}

	/**
	 * @brief start event loop
	 * @details if this is called in the current thread. it will block current thread until the end of time loop
	 * see onBeforeStart
	 */
	bool start(bool newThread = true)
	{
		std::unique_lock<std::mutex> lck(_sMutex, std::defer_lock);
		lck.lock();
		if (_status != StatusInit)
		{
			__LOG(error, " conneciton status is not Init state, status is :" << loopStatus::toString(_status));
			return false;
		}
		lck.unlock();
		if (!onBeforeStart())
		{
			__LOG(error, "onBeforeStart return fail");
			return false;
		}
		if (newThread)
		{
			// take care here
			_thread = std::make_shared<std::thread>(std::bind(&loop::_run, shared_from_this()));
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
		std::unique_lock<std::mutex> lck(_sMutex, std::defer_lock);
		lck.lock();
		if (StatusFinished == _status)
		{
			return;
		}
		lck.unlock();
		waiting ? event_base_loopexit(_base, NULL) : event_base_loopbreak(_base);
		onAfterStop();
	}

protected:
	virtual bool onBeforeStart()
	{
		return true;
	}
	virtual void onBeforeLoop() {}
	virtual void onAfterLoop() {}
	virtual void onAfterStop() {}

private:
	void _run()
	{
		std::unique_lock<std::mutex> lck(_sMutex, std::defer_lock);
		lck.lock();
		_status = StatusRunning;
		lck.unlock();
		onBeforeLoop();
		event_base_loop(_base, 0);
		onAfterLoop();
		lck.lock();
		_status = StatusFinished;
	}

private:
	event_base *_base;
	std::shared_ptr<std::thread> _loopThread;
	loopStatus _status;
	std::mutex _sMutex;
};

} // namespace loop
