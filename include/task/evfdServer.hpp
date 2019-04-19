#pragma once
#include "loop/loop.hpp"
namespace task
{
class evfdServer
{
public:
    typedef void (*evCb)(evutil_socket_t fd, short event, void *args);
    evfdServer() = delete;
    // Note:!! please make sure your fd is non-blocking
    // for example: int ev_fd = eventfd(0, EFD_NONBLOCK|EFD_CLOEXEC);
    // Note:!! please make sure that you pass in a right envent base loop
    // event base is not changeable, if you want to change it, please kill this object and start a new _one
    // will add some some check later.....
    evfdServer(std::shared_ptr<loop::loop> loopIn, int efd, evCb cb, void *arg) : eventFd(efd), _one(1), _loop(loopIn), eventCallback(cb), _arg(arg)
    {
    }
    virtual ~evfdServer()
    {
        if (NULL != _event)
        {
            event_free(_event);
            _event = NULL;
        }
    }

    bool init()
    {
        if (_loop.expired())
        {
            __LOG(error, "loop is invalid!");
            return false;
        }
        _event = event_new((_loop.lock())->ev(), eventFd, EV_READ | EV_PERSIST, eventCallback, _arg);
        if (NULL == _event)
        {
            return false;
        }
        if (0 != event_add(_event, NULL))
        {
            return false;
        }
        return true;
    }

private:
    int eventFd;
    uint64_t _one;
    std::weak_ptr<loop::loop> _loop;
    evCb eventCallback;
    struct event *_event;
    void *_arg;
};
} // namespace task