#pragma once
enum class taskMsgType : std::uint32_t
{
TASK_REDIS_FORMAT_RAW,
TASK_REDIS_RAW,

TASK_REDIS_ADD_CONN,
TASK_REDIS_DEL_CONN,
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
typedef std::queue<taskMsg> TASK_QUEUE;
