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
#include <memory>
#include <gtest/gtest.h>
#include "hiredis/hiredis.h"
#include <hiredis/async.h>

#include "redisAsyncClient.hpp"
#include "util.hpp"
class basicDevelopTestRR : public testing::Test
{
protected:
    virtual void SetUp() override
    {

        _aclient_sptr = std::make_shared<redisAsyncClient>();

        configCenter::cfgPropMap _config;
        _config[PROP_HOST] = "127.0.0.1";
        configCenter::configCenter<void *>::instance()->setProperties(_aclient_sptr->getThis(), _config);

        _aclient_sptr->init();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    virtual void TearDown() override
    {
        // ...
        _aclient_sptr->cleanUp();
        _aclient_sptr.reset();
    }
    std::shared_ptr<redisAsyncClient> _aclient_sptr;
};

TEST_F(basicDevelopTestRR, put)
{
    bool ret = _aclient_sptr->put(std::string("hello"), std::string("world"), NULL, getCallback);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_TRUE(ret);
}
TEST_F(basicDevelopTestRR, get)
{
    bool ret = _aclient_sptr->get(std::string("hello"), nullptr, NULL, getCallback);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_TRUE(ret);
}
TEST_F(basicDevelopTestRR, del)
{
    bool ret = _aclient_sptr->del(std::string("hello"), nullptr, NULL, getCallback);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_TRUE(ret);
}
int main(int argc, char *argv[])
{
    MEDIS_GLOB_SIPMLE_INIT();
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    MEDIS_GLOB_CLEAN_UP();
    return ret;
}
