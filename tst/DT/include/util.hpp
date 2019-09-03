#pragma once
#include <gtest/gtest.h>
#include "redisAsyncClient.hpp"
void testGetCallback(redisAsyncContext *c, void *r, void *privdata);