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
#pragma once
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "util/medisType.hpp"
#include <hiredis/adapters/libevent.h>
#include <queue>

namespace task
{
enum class taskMsgType : std::uint32_t
{
    TASK_REDIS_FORMAT_RAW,
    TASK_REDIS_RAW,

    TASK_REDIS_ADD_CONN,

    TASK_REDIS_DEL_CONN,
    // when the first connection avaliable, need to clean the message queueu.
    TASK_REDIS_CONN_AVALIABLE,
    TASK_MSG_MAX
};

struct taskMsg
{
    taskMsgType type;
 //   std::uint32_t seq_id;
 //   std::string from;
//    std::string to;
    DBW_ANY body;
};

struct TASK_REDIS_FORMAT_RAW_MSG_BODY
{
    redisCallbackFn *fn;
    std::string body;
    void *usr_data;
};
struct TASK_REDIS_RAW_MSG_BODY
{
    redisCallbackFn *fn;
    std::string body;
    void *usr_data;
};

using TASK_QUEUE = std::queue<taskMsg>;
} // namespace task