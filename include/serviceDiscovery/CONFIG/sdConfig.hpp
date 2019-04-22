#pragma once
#include "serviceDiscovery/serviceDiscovery.hpp"
#include "timer/timerManager.hpp"
#include "configCenter/configUtil.hpp"
#include "util/defs.hpp"
namespace serviceDiscovery
{
template <typename connInfo>
class sdConfig
{
public:
    sdConfig(std::shared_ptr<timer::timerManager> timer_manager) : serviceDiscovery<connInfo>(timer_manager) {}
    virtual bool init()
    {

        std::string connHost = configCenter::configCenter<void *>::instance()->get_properties_fields(get_genetic_gene(), PROP_HOST, DEFAULT_PROP_HOST);
        std::string connPort = configCenter::configCenter<void *>::instance()->get_properties_fields(get_genetic_gene(), PROP_PORT, DEFAULT_PROP_PORT);

        // build connInfo
        connInfo _connInfo;

        _connInfo.type = dbw::CONN_TYPE::IP;
        _connInfo.ip = connHost;
        _connInfo.port = connPort;
        std::list<connInfo> _connInfo_list;
        _connInfo_list.push_back(_connInfo);

        return updateConnInfo(_connInfo_list);
    }
};
} // namespace serviceDiscovery
