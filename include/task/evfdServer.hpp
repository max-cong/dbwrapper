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
#include "loop/loop.hpp"
#include "util/nonCopyable.hpp"

namespace task
{
class evfdServer : public nonCopyable
{
public:
    typedef void (*evCb)(evutil_socket_t fd, short event, void *args);
    evfdServer() = delete;
    // Note:!! please make sure your fd is non-blocking

    // Note:!! please make sure that you pass in a right envent base loop
    // event base is not changeable, if you want to change it, please kill this object and start a new _one
    // will add some some check later.....
    evfdServer(std::shared_ptr<loop::loop> loopIn, int efd, evCb cb, void *arg) : eventFd(efd), _loop(loopIn), eventCallback(cb), _arg(arg)
    {
    }

    bool init()
    {
        if (_loop.expired())
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "loop is invalid!");
            }
            return false;
        }
        _event_sptr.reset(event_new((_loop.lock())->ev(), eventFd, EV_READ | EV_PERSIST, eventCallback, _arg), [](event *innerEvent) {
            if (NULL != innerEvent)
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "[evfdServer] event free is called!");
                }
                event_del(innerEvent);
                event_free(innerEvent);
                innerEvent = NULL;
            }
        });
        if (!_event_sptr)
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "create event fail!");
            }
            return false;
        }
        if (0 != event_add(_event_sptr.get(), NULL))
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "event add return fail!");
            }
            return false;
        }
        
        return true;
    }

private:
    int eventFd;

    std::weak_ptr<loop::loop> _loop;
    evCb eventCallback;

    std::shared_ptr<event> _event_sptr;
    void *_arg;
};
} // namespace task