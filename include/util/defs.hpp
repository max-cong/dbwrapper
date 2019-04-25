#pragma once
#include "lbStrategy/lbStrategy.hpp"
#include "heartbeat/heartbeat.hpp"
namespace dbw
{
struct redisContext
{
    int _priority;
    redisAsyncContext *_ctx;
    std::shared_ptr<heartBeat::heartBeat> _hb;
    std::shared_ptr<lbStrategy::lbStrategy<redisAsyncContext *>> _lbs;
    std::shared_ptr<timer::timerManager> _retryTimerManager;
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
    CONN_INFO() : type(CONN_TYPE::IP), priority(1) {}
    CONN_TYPE type;
    std::string ip;
    std::string path;
    unsigned short port;
    int priority;
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
    static contextSaver<OBJ, RDS_CTX> *instance()
    {
        static contextSaver<OBJ, RDS_CTX> *ins = new contextSaver<OBJ, RDS_CTX>();
        return ins;
    }
    void save(OBJ obj, RDS_CTX gene)
    {
        _geneMap[obj] = gene;
    }
    void del(OBJ obj)
    {
        _geneMap.erase(obj);
    }
    std::pair<RDS_CTX, bool> getCtx(const OBJ obj)
    {
        if (_geneMap.find(obj) != _geneMap.end())
        {
            return std::make_pair(_geneMap[obj], true);
        }
        RDS_CTX gene;
        return std::make_pair(gene, false);
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
    void save(OBJ obj, RDS_TASK_SPTR gene)
    {
        _geneMap[obj] = gene;
    }
    void del(OBJ obj)
    {
        _geneMap.erase(obj);
    }
    std::pair<RDS_TASK_SPTR, bool> getTask(const OBJ obj)
    {
        if (_geneMap.find(obj) != _geneMap.end())
        {
            return std::make_pair(_geneMap[obj], true);
        }
        RDS_TASK_SPTR gene;
        return std::make_pair(gene, false);
    }

    std::map<OBJ, RDS_TASK_SPTR> _geneMap;
};



} // namespace dbw
