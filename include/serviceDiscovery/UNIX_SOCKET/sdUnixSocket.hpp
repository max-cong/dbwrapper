#pragma once
#include "serviceDiscovery/serviceDiscovery.hpp"
namespace serviceDiscovery
{
template <typename connInfo>
class sdUnixSocket : public serviceDiscovery<connInfo>
{
public:
	sdUnixSocket(std::shared_ptr<timer::timerManager> timer_manager = nullptr) : serviceDiscovery<connInfo>(timer_manager)
	{
	}
	virtual bool init()
	{
		// get unix path
		std::string _unix_socket_path = configCenter::configCenter<void *>::instance()->get_properties_fields(get_genetic_gene(), PROP_UNIX_PATH, DEFAULT_REDIS_UNIX_PATH);

		// build connInfo
		connInfo _connInfo;

		_connInfo.type = dbw::CONN_TYPE::UNIX_SOCKET;
		_connInfo.path = _unix_socket_path;

		std::list<connInfo> _connInfo_list;
		_connInfo_list.push_back(_connInfo);

		return updateConnInfo(_connInfo_list);
	}
};
} // namespace serviceDiscovery