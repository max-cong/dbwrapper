#include "redisAsyncClient.hpp"
#include <gtest/gtest.h>
void getCallback(redisAsyncContext *c, void *r, void *privdata);
