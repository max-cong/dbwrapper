#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "redisAsyncClient.hpp"
#include <hiredis.h>

void getCallback(redisAsyncContext *c, void *r, void *privdata)
{
    redisReply *reply = (redisReply *)r;
    if (reply == NULL)
    {
        if (c->errstr)
        {
            printf("errstr: %s\n", c->errstr);
        }
        return;
    }
    printf("argv[%s]: %s\n", (char *)privdata, reply->str);
}
int main()
{
    set_logLevel(loggerIface::logLevel::debug);
    redisAsyncClient aclient;

    configCenter::cfgPropMap _config;
    _config[PROP_HOST] = "127.0.0.1";

    configCenter::configCenter<void *>::instance()->set_properties(aclient.getThis(), _config);
    aclient.init();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    aclient.put("hello", "world", NULL, getCallback);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "hello world" << std::endl;
}