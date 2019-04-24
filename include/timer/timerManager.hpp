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
#include "loop/loop.hpp"
#include "timer.hpp"
#include <unordered_map>
#include <atomic>
#include <thread>
#include <algorithm>
#include <mutex>
#include "logger/logger.hpp"
namespace timer
{
class timerManager
{
public:
	timerManager() = delete;
	explicit timerManager(std::shared_ptr<loop::loop> loopIn) : _uniqueIDAtomic(0), _timerMap(), _loop(loopIn)
	{
	}
	virtual ~timerManager()
	{
	}
	bool stop()
	{
		std::lock_guard<std::mutex> lck(_tMutex);
		_timerMap.clear();
		return true;
	}
	timer::ptr_p getTimer()
	{
		unsigned long tid = getUniqueID();
		if(_loop.expired())
		{
			__LOG(error, "loop is invalid!");
			return nullptr;
		}
		timer::ptr_p tmp_ptr(new timer(_loop.lock()));
		tmp_ptr->setTid(tid);
		{
			std::lock_guard<std::mutex> lck(_tMutex);
			_timerMap[tid] = tmp_ptr;
		}
		return tmp_ptr;
	}
	bool killTimer(unsigned long timerID)
	{
		std::lock_guard<std::mutex> lck(_tMutex);
		_timerMap.erase(timerID);
		return true;
	}
	bool killTimer(timer::ptr_p timer_sptr)
	{
		return killTimer(timer_sptr->getTid());
	}


private:
	unsigned long getUniqueID()
	{
		return (_uniqueIDAtomic++);
	}
	std::mutex _tMutex;
	std::atomic<unsigned long> _uniqueIDAtomic;
	std::unordered_map<unsigned long, timer::ptr_p> _timerMap;
	std::weak_ptr<loop::loop> _loop;
};
} // namespace timer
