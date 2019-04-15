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
	loop();
	virtual ~loop();

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
		return _status;
	}

	/**
	 * @brief start event loop
	 * @details if this is called in the current thread. it will block current thread until the end of time loop
	 * see onBeforeStart
	 */
	bool start(bool newThread = true);

	void wait();

	/** if run with new thread, will stop the new thread
	 * waiting: if call the active callback before exit
	 */
	void stop(bool waiting = true);

  protected:
	virtual bool onBeforeStart();

	virtual void onBeforeLoop();

	virtual void onAfterLoop();

	virtual void onAfterStop();

  private:
	void _run();

  private:
	event_base *_base;

	std::shared_ptr<std::thread> _loopThread;

	loopStatus _status;

  private:
	std::mutex _sMutex;
};

} // namespace loop
