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

#include "translib/loop.h"



namespace loop
{

loop::loop() : _base(NULL),
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

loop::~loop()
{
    if (_thread)
    {
        _thread.reset();
    }

    if (NULL != _base)
    {
        event_base_free(_base);
        _base = NULL;
    }
}

bool loop::start(bool newThread)
{
    if (_status != StatusInit)
    {
        __LOG(error, " conneciton status is not Init state, status is :" << loopStatus::toString(_status));
        return false;
    }

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

void loop::wait()
{
    if ( _thread && StatusFinished != _status)
    {
        _thread->join();
    }
}

void loop::stop(bool waiting)
{
    if (StatusFinished == _status)
    {
        return;
    }

    waiting ? event_base_loopexit(_base, NULL) : event_base_loopbreak(_base);
    onAfterStop();
}

void loop::_run()
{
    _status = StatusRunning;
  
    onBeforeLoop();
    event_base_loop(_base, 0);
    onAfterLoop();
    
    _status = StatusFinished;
}

bool loop::onBeforeStart()
{
    return true;
}

void loop::onBeforeLoop()
{
}

void loop::onAfterLoop()
{
}

void loop::onAfterStop()
{
}



} // namespace loop
