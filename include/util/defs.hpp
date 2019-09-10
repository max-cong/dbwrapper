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
#include "lbStrategy/lbStrategy.hpp"
#include "timer/timerManager.hpp"
#include "logger/logger.hpp"
namespace medis
{
enum class retStatus : std::uint32_t
{
    SUCCESS = 0,
    FAIL,
    NO_ENTRY,
    FIRST_ENTRY
};
enum class CONN_TYPE : std::uint32_t
{
    IP,
    UNIX_SOCKET,
    TYPE_MAX
};

class CONN_INFO
{
public:
    CONN_INFO() : type(CONN_TYPE::IP), priority(1), hbTime(0) {}
    CONN_TYPE type;
    std::string ip;
    std::string path;
    unsigned short port;
    int priority;
    unsigned int hbTime;
    bool operator==(const CONN_INFO &rhs)
    {
        return this->type == rhs.type && this->ip == rhs.ip && this->path == rhs.path && this->port == rhs.port && this->priority == rhs.priority;
    }
};

// this is singleton to save redisAsyncContext<->redisContext
template <typename OBJ, typename RDS_CTX>
class contextSaver
{
public:
    void save(OBJ obj, RDS_CTX gene)
    {
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "save one object in the context saver");
        }
        _geneMap[obj] = gene;
    }
    void del(OBJ obj)
    {
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "delete one object");
        }
        _geneMap.erase(obj);
    }
    std::list<RDS_CTX> getIpPortThenDel(std::string ip, unsigned short port)
    {
        std::list<RDS_CTX> ctxList;
        auto iter = _geneMap.begin();
        while (iter != _geneMap.end())
        {
            if (iter->second->ip == ip && iter->second->port == port)
            {
                ctxList.push_back(iter->second);
                iter = _geneMap.erase(iter);
            }
            else
            {
                iter++;
            }
        }
        return ctxList;
    }
    DBW_OPT<RDS_CTX> getCtx(const OBJ obj)
    {
        if (_geneMap.find(obj) != _geneMap.end())
        {
            return _geneMap[obj];
        }

        return DBW_NONE_OPT;
    }

    std::map<OBJ, RDS_CTX> _geneMap;
};
// this is gene <->task
template <typename OBJ, typename RDS_TASK_SPTR>
class taskSaver
{
public:
    static taskSaver<OBJ, RDS_TASK_SPTR> *instance()
    {
        static taskSaver<OBJ, RDS_TASK_SPTR> *ins = new taskSaver<OBJ, RDS_TASK_SPTR>();
        return ins;
    }
    static void distroy(taskSaver<OBJ, RDS_TASK_SPTR> *ins)
    {
        if (CHECK_LOG_LEVEL(warn))
        {
            __LOG(warn, "[taskSaver] distroy!");
        }
        if (ins)
        {
            if (CHECK_LOG_LEVEL(warn))
            {
                __LOG(warn, "[taskSaver] item in the map : " << (ins->_geneMap).size());
            }
            ins->_geneMap.clear();
            delete ins;
            ins = NULL;
        }
    }
    void save(OBJ obj, RDS_TASK_SPTR gene)
    {
        _geneMap[obj] = gene;
    }
    void del(OBJ obj)
    {
        if (_geneMap.find(obj) != _geneMap.end())
        {
            if (CHECK_LOG_LEVEL(warn))
            {
                __LOG(warn, "[taskSaver] now stop one task");
            }
            _geneMap[obj]->stop();
        }
        _geneMap.erase(obj);
    }
    DBW_OPT<RDS_TASK_SPTR> getTask(const OBJ obj)
    {
        if (_geneMap.find(obj) != _geneMap.end())
        {
            return _geneMap[obj];
        }
        return DBW_NONE_OPT;
    }

    std::map<OBJ, RDS_TASK_SPTR> _geneMap;
};

} // namespace medis
