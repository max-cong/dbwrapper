#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <memory>
#include "redisAsyncClient.hpp"
#include "hiredis/hiredis.h"

#include <gtest/gtest.h>
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
    __LOG(debug, "private data is : " << (void *)privdata << ", string is : " << reply->str);
}
class redisAsyncClientTest : public testing::Test
{
protected:
    virtual void SetUp() override
    {
        set_logLevel(loggerIface::logLevel::debug);
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
    }
    std::shared_ptr<redisAsyncClient> _aclient_sptr;
};

TEST_F(redisAsyncClientTest, put)
{
    _aclient_sptr->put("hello", "world", NULL, getCallback);
    EXPECT_EQ(0, 0);
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
