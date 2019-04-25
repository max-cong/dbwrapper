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
    evfdServer(std::shared_ptr<loop::loop> loopIn, int efd, evCb cb, void *arg) : eventFd(efd), _one(1), _loop(loopIn), eventCallback(cb), _arg(arg)
    {
    }

    bool init()
    {
        if (_loop.expired())
        {
            __LOG(error, "loop is invalid!");
            return false;
        }
        _event_sptr.reset(event_new((_loop.lock())->ev(), eventFd, EV_READ | EV_PERSIST, eventCallback, _arg), [](event *innerEvent) {
            if (NULL != innerEvent)
            {
                event_free(innerEvent);
                innerEvent = NULL;
            }
        });
        if (!_event_sptr)
        {
            return false;
        }
        if (0 != event_add(_event_sptr.get(), NULL))
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

    std::shared_ptr<event> _event_sptr;
    void *_arg;
};
} // namespace task