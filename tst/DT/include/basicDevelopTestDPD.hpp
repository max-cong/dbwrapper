#include "util.hpp"
class basicDevelopTestDPD : public testing::Test
{
protected:
    virtual void SetUp() override
    {

        _aclient_sptr = std::make_shared<redisAsyncClient>();

        configCenter::cfgPropMap _config;
        _config[PROP_HOST] = "127.0.0.1";
        _config[PROP_LOAD_BALANCE_STRATEGY] = "DPD";
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

TEST_F(basicDevelopTestDPD, put)
{
    bool ret = _aclient_sptr->put(std::string("hello"), std::string("world"), NULL, getCallback);
    EXPECT_TRUE(ret);
}
TEST_F(basicDevelopTestDPD, get)
{
    bool ret = _aclient_sptr->get(std::string("hello"), nullptr, NULL, getCallback);
    EXPECT_TRUE(ret);
}
TEST_F(basicDevelopTestDPD, del)
{
    bool ret = _aclient_sptr->del(std::string("hello"), nullptr, NULL, getCallback);
    EXPECT_TRUE(ret);
}