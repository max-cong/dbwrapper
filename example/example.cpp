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
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "redisAsyncClient.hpp"
#include "hiredis/hiredis.h"
int i = 0;
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
    __LOG(debug, "private data is : " << (void *)privdata << ", string is : " << reply->str << ", index is : " << i);
}
int main()
{
    set_logLevel(loggerIface::logLevel::debug);
    redisAsyncClient aclient;

    configCenter::cfgPropMap _config;
    _config[PROP_HOST] = "127.0.0.1";
    _config[PROP_PORT] = "6379";
    configCenter::configCenter<void *>::instance()->setProperties(aclient.getThis(), _config);
    __LOG(debug, "start redis async client and set config with gene : " << (void *)aclient.getThis());

    aclient.init();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // for (int i = 0; i < 10000; i++)
    {
        aclient.put(std::string("hello"), std::string("world"), NULL, getCallback);
        aclient.get(std::string("hello"), nullptr, NULL, getCallback);
        aclient.del(std::string("hello"), nullptr, NULL, getCallback);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50000));
}