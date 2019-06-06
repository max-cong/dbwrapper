#pragma once
#include <gtest/gtest.h>
#include "redisAsyncClient.hpp"
void getCallback(redisAsyncContext *c, void *r, void *privdata);