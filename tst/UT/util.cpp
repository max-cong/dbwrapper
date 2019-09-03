#include "util.hpp"
void getCallback(redisAsyncContext *c, void *r, void *privdata)
{
    redisReply *reply = (redisReply *)r;
    if (reply == NULL)
    {
        if (c->errstr)
        {
            if (CHECK_LOG_LEVEL(debug))
            {
                __LOG(debug, "errstr: %s" << c->errstr);
            }
        }
        return;
    }
    if (CHECK_LOG_LEVEL(debug))
    {
        __LOG(debug, "private data is : " << (void *)privdata << ", string is : " << reply->str);
    }
}