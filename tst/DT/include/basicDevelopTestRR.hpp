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
        _aclient_sptr->cleanUp();
        // ...
    }
    std::shared_ptr<redisAsyncClient> _aclient_sptr;
};

TEST_F(basicDevelopTestRR, put)
{
    bool ret = _aclient_sptr->put(std::string("hello"), std::string("world"), NULL, testGetCallback);
    EXPECT_TRUE(ret);
}
TEST_F(basicDevelopTestRR, get)
{
    bool ret = _aclient_sptr->get(std::string("hello"), nullptr, NULL, testGetCallback);
    EXPECT_TRUE(ret);
}
TEST_F(basicDevelopTestRR, del)
{
    bool ret = _aclient_sptr->del(std::string("hello"), nullptr, NULL, testGetCallback);
    EXPECT_TRUE(ret);
}