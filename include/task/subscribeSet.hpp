#pragma once
#include "logger/logger.hpp"
#include "task/taskUtil.hpp"

namespace task
{
class taskMsgCmp
{
public:
    bool operator()(const std::pair<std::shared_ptr<taskMsg>, redisAsyncContext *> &lhs, const std::pair<std::shared_ptr<taskMsg>, redisAsyncContext *> &rhs) const
    {
        std::shared_ptr<TASK_REDIS_FORMAT_RAW_MSG_BODY> lmsg = DBW_ANY_CAST<std::shared_ptr<TASK_REDIS_FORMAT_RAW_MSG_BODY>>((lhs.first)->body);
        std::shared_ptr<TASK_REDIS_FORMAT_RAW_MSG_BODY> rmsg = DBW_ANY_CAST<std::shared_ptr<TASK_REDIS_FORMAT_RAW_MSG_BODY>>((rhs.first)->body);
        return (lmsg->body < rmsg->body);
    }
};
using subscribeSet_t = std::set<std::pair<std::shared_ptr<taskMsg>, redisAsyncContext *>, taskMsgCmp>;
using subscribeSetSptr_t = std::shared_ptr<subscribeSet_t>;
class subscribeSet
{
public:
    bool update(std::shared_ptr<taskMsg> msg, redisAsyncContext *aCtx)
    {
        auto msgCtxPair = std::make_pair(msg, aCtx);
        return _subsSet.insert(msgCtxPair).second;
    }
    // update related async client to NULL.
    bool update(redisAsyncContext *aCtx)
    {
        for (auto it : _subsSet)
        {
            if (it.second == aCtx)
            {
                it.second = NULL;
            }
        }
        return true;
    }
    bool del(std::string command)
    {

        for (auto it = _subsSet.begin(); it != _subsSet.end(); it++)
        {
            if (DBW_ANY_CAST<std::shared_ptr<TASK_REDIS_FORMAT_RAW_MSG_BODY>>((*it).first->body)->body == command)
            {
                _subsSet.erase(it);
                return true;
            }
        }
        return true;
    }
    DBW_OPT<std::pair<std::shared_ptr<taskMsg>, redisAsyncContext *>> get(std::string command)
    {
        for (auto it = _subsSet.begin(); it != _subsSet.end(); it++)
        {
            if (DBW_ANY_CAST<std::shared_ptr<TASK_REDIS_FORMAT_RAW_MSG_BODY>>((*it).first->body)->body == command)
            {
                return *it;
            }
        }
        return DBW_NONE_OPT;
    }
    subscribeSet_t getToSub()
    {
        subscribeSet_t retSet;
        for (auto it = _subsSet.begin(); it != _subsSet.end(); it++)
        {
            if (!(*it).second)
            {
                retSet.insert(*it);
            }
        }
        return retSet;
    }

private:
    subscribeSet_t _subsSet;
};
} // namespace task