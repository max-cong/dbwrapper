#include "util.hpp"
void testGetCallback(redisAsyncContext *c, void *r, void *privdata)
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
    if (CHECK_LOG_LEVEL(error))
    {
        __LOG(error, "private data is : " << (void *)privdata << ", string is : " << reply->str);
    }
}