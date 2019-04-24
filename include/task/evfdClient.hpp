#pragma once
#include <unistd.h>
namespace task
{
class evfdClient
{
public:
    evfdClient() = delete;
    // Note:!! please make sure your fd is non-blocking
    
    explicit evfdClient(int efd) : _evfd(efd),_one(1)
    {
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
