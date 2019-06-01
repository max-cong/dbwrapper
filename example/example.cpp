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

#include <ctime>
#include <stdlib.h>
#include "redisAsyncClient.hpp"
#include "hiredis/hiredis.h"

#include <random>
std::random_device seeder;
std::mt19937 rng(seeder());
std::uniform_int_distribution<int> gen(0, 1000); // uniform, unbiased

int i = 0;
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
    i++;
    if (CHECK_LOG_LEVEL(debug))
    {
        __LOG(debug, "private data is : " << (void *)privdata << ", string is : " << reply->str << ", index is : " << i);
    }
    std::cout << "receive response with index : " << i << std::endl;
}
int main()
{
    std::unique_ptr<boost_logger> boostloggerUptr(new boost_logger());
    INIT_LOGGER(boostloggerUptr);
    SET_LOG_LEVEL(debug);

    redisAsyncClient aclient;
    int loop_time = 100;
    configCenter::cfgPropMap _config;
    _config[PROP_HOST] = "127.0.0.1";
    _config[PROP_PORT] = "6379";
    configCenter::configCenter<void *>::instance()->setProperties(aclient.getThis(), _config);
    if (CHECK_LOG_LEVEL(debug))
    {
        __LOG(debug, "start redis async client and set config with gene : " << (void *)aclient.getThis());
    }

    aclient.init();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::time_t startTime = std::time(nullptr);
    std::cout << "start time : " << std::asctime(std::localtime(&startTime))
              << startTime << "\n";
    for (int i = 0; i < loop_time; i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(gen(rng)));
        aclient.put(std::string("hello"), std::string("world"), NULL, getCallback);
        aclient.get(std::string("hello"), nullptr, NULL, getCallback);
        aclient.del(std::string("hello"), nullptr, NULL, getCallback);
    }
    std::time_t stopTime = std::time(nullptr);
    std::cout << "stop time : " << std::asctime(std::localtime(&stopTime))
              << stopTime << "\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    std::cout << "total receive response is : " << i << std::endl;
}
