#pragma once
#include "serviceDiscovery/serviceDiscovery.hpp"
namespace serviceDiscovery
{
template <typename connInfo>
class sdUnixSocket : public serviceDiscovery<connInfo>
{
public:
	explicit sdUnixSocket(std::shared_ptr<loop::loop> loopIn) : serviceDiscovery<connInfo>(loopIn)
	{
	}
	virtual bool init()
	{
		// get unix path
		std::string _unix_socket_path = configCenter::configCenter<void *>::instance()->getPropertiesField(this->getGeneticGene(), PROP_UNIX_PATH, DEFAULT_REDIS_UNIX_PATH);

		// build connInfo
		connInfo _connInfo;

		_connInfo.type = medis::CONN_TYPE::UNIX_SOCKET;
		_connInfo.path = _unix_socket_path;

		std::list<connInfo> _connInfo_list;
		_connInfo_list.push_back(_connInfo);

		return this->updateConnInfo(_connInfo_list);
	}
};
} // namespace serviceDiscovery