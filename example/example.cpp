#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "redisAsyncClient.hpp"
#include "hiredis/hiredis.h"
int i =0;
void getCallback(redisAsyncContext *c, void *r, void *privdata)
{
    redisReply *reply = (redisReply *)r;
    if (reply == NULL)
    {
        if (c->errstr)
        {
            __LOG(debug, "errstr: %s" << c->errstr);
        }
        return;
    }
    i++;
    __LOG(debug, "private data is : " << (void *)privdata << ", string is : " << reply->str<<", index is : "<<i);
}
int main()
{
    set_logLevel(loggerIface::logLevel::debug);
    redisAsyncClient aclient;

    configCenter::cfgPropMap _config;
    _config[PROP_HOST] = "127.0.0.1";
    configCenter::configCenter<void *>::instance()->setProperties(aclient.getThis(), _config);

    aclient.init();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    for (int i = 0; i < 10000; i++)
    {
        aclient.put("hello", "world", NULL, getCallback);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50000));
}