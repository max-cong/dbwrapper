#pragma once
#include "service_discovery/service_discovery.hpp"

class srvcUnixSocket : public service_discovery<connInfo>
{
  public:
	srvcUnixSocket(std::shared_ptr<translib::TimerManager> timer_manager = nullptr) : service_discovery<connInfo>(timer_manager)
	{
	}
	virtual bool init()
	{
		// get unix path
		std::string _unix_socket_path = config_center<void *>::instance()->get_properties_fields(get_genetic_gene(), PROP_UNIX_PATH, DEFAULT_REDIS_UNIX_PATH);
		// build connInfo
		connInfo _connInfo;

		_connInfo.type = conn_type::UNIX_SOCKET;
		_connInfo.path = _unix_socket_path;

		std::list<connInfo> _connInfo_list;
		_connInfo_list.push_back(_connInfo);

		setConnInfoList(_connInfo_list);
		return true;
	}
};