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
    std::uint32_t seq_id;
    std::string from;
    std::string to;
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