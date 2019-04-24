#pragma once
#include <unistd.h>
namespace task
{
class evfdClient
{
public:
    evfdClient() = delete;
    // Note:!! please make sure your fd is non-blocking
    // for example: int ev_fd = eventfd(0, EFD_NONBLOCK|EFD_CLOEXEC);
    explicit evfdClient(int efd) : _one(1)
    {
        _evfd = efd;
        __LOG(debug, "event fd is " << _evfd);
    }
    virtual ~evfdClient() {}

    bool send(uint64_t size = 1)
    {
        _one = size;
        int ret = write(_evfd, &_one, sizeof(_one));
        if (ret != sizeof(_one))
        {
            __LOG(error, "write event fd : " << _evfd << " fail");
            return false;
        }
        return true;
    }

private:
    int _evfd;
    uint64_t _one;
};
} // namespace task
