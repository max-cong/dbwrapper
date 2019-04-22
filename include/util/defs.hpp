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
};

enum class CONN_TYPE : std::uint32_t
{
    IP,
    UNIX_SOCKET,
    TYPE_MAX
};

struct CONN_INFO
{
    CONN_INFO() : priority(1), type(CONN_TYPE::IP) {}
    CONN_TYPE type;
    std::string ip;
    std::string path;
    unsigned short port;
    int priority;
};
// this is singleton to save redisAsyncContext<->redisContext
template <typename OBJ = void *, typename RDS_CTX = std::shared_ptr<redisContext>>
class contextSaver
{
public:
    static contextSaver *instance()
    {
        static contextSaver *ins = new contextSaver();
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
    std::pair<RDS_CTX, bool> get(OBJ obj)
    {
        if (_geneMap.find() != _geneMap.end())
        {
            return std::make_pair(_geneMap[obj], true);
        }
        RDS_CTX gene;
        return std::make_pair(gene, false);
    }

    std::map<OBJ, RDS_CTX> _geneMap;
};
} // namespace dbw
