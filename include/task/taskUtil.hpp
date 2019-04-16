#pragma once
enum class taskMsgType : std::uint32_t
{


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
