#pragma once

#include "loop/loop.hpp"
#include "hiredis/async.h"
#include "buildRedisCommand/buildRedisCommand.hpp"
#include "logger/logger.hpp"
#include "task/task.hpp"
#include "util/nonCopyable.hpp"

#include <string>
#include <memory>

class redisAsyncClient : public nonCopyable
{
public:
    bool init()
    {

        _loop_sptr.reset(new loop::loop(), [](loop::loop *innerLoop) {
            if (innerLoop)
            {
                innerLoop->stop(true);
                delete innerLoop;
            }
        });
        if (!_loop_sptr)
        {
            __LOG(error, "init loop fail");
            return false;
        }

        // start task
        _task_sptr = std::make_shared<task::taskImp>(_loop_sptr);

        if (!_task_sptr)
        {
            __LOG(error, "create task fail");
            return false;
        }
        _task_sptr->init();
        return _loop_sptr->start(true);
    }
    bool put(std::string key, std::string value, void *usr_data, redisCallbackFn *fn)
    {
        if (!getConnStatus())
        {
            return false;
        }
        std::string command2send = buildRedisCommand::buildRedisCommand<std::string, std::string>::get_format_command(REDIS_MSG_TYPE::TASK_REDIS_PUT, key, value);
        __LOG(debug, "get command :\n"
                         << command2send);
        return send_format_raw_command(command2send, usr_data, fn);
    }

    bool send_format_raw_command(std::string command, void *usr_data, redisCallbackFn *fn)
    {
        task::TASK_REDIS_FORMAT_RAW_MSG_BODY msg;
        msg.fn = fn;
        msg.body = command;
        msg.usr_data = usr_data;
        _task_sptr->sendMsg(task::taskMsgType::TASK_REDIS_FORMAT_RAW, msg);
        return true;
    }
    bool send_raw_command(std::string command, void *usr_data, redisCallbackFn *fn)
    {
        task::TASK_REDIS_RAW_MSG_BODY msg;
        msg.fn = fn;
        msg.body = command;
        msg.usr_data = usr_data;
        return _task_sptr->sendMsg(task::taskMsgType::TASK_REDIS_RAW, msg);
    }
    void *getThis()
    {
        return (void *)this;
    }
    bool getConnStatus()
    {
        return _task_sptr->getConnStatus();
    }
    std::shared_ptr<loop::loop> _loop_sptr;
    std::shared_ptr<task::taskImp> _task_sptr;
};
