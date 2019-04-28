#pragma once
#include "serviceDiscovery/serviceDiscovery.hpp"
#include "timer/timerManager.hpp"
#include "configCenter/configUtil.hpp"
#include "util/defs.hpp"
namespace serviceDiscovery
{
template <typename connInfo>
class sdConfig : public serviceDiscovery<connInfo>
{
public:
    explicit sdConfig(std::shared_ptr<loop::loop> loopIn) : serviceDiscovery<connInfo>(loopIn) {}
    virtual bool init()
    {

        std::string connHost = configCenter::configCenter<void *>::instance()->getPropertiesField(this->getGeneticGene(), PROP_HOST, DEFAULT_HOST);
        std::string connPort = configCenter::configCenter<void *>::instance()->getPropertiesField(this->getGeneticGene(), PROP_PORT, DEFAULT_PORT);
        __LOG(debug, "get host : " << connHost << ", port is : " << connPort);
        // build connInfo
        connInfo _connInfo;

        _connInfo.type = medis::CONN_TYPE::IP;
        _connInfo.ip = connHost;

        std::string::size_type sz; // alias of size_t

        int port = std::stoi(connPort, &sz);

        _connInfo.port = port;
        std::list<connInfo> _connInfo_list;
        _connInfo_list.push_back(_connInfo);

        return this->updateConnInfo(_connInfo_list);
    }
};
} // namespace serviceDiscovery
