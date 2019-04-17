#pragma once
#include "service_discovery/service_discovery.hpp"

class unix_socket_service_discovery : public service_discovery<conn_info>
{
  public:
	unix_socket_service_discovery(std::shared_ptr<translib::TimerManager> timer_manager = nullptr) : service_discovery<conn_info>(timer_manager)
	{
	}
	virtual bool init()
	{
		std::lock_guard<std::recursive_mutex> lck(_mutex);
		// get unix path
		std::string _unix_socket_path = config_center<void *>::instance()->get_properties_fields(get_genetic_gene(), PROP_UNIX_PATH, DEFAULT_REDIS_UNIX_PATH);
		// build conn_info
		conn_info _conn_info;

		_conn_info.type = conn_type::UNIX_SOCKET;
		_conn_info.path = _unix_socket_path;

		std::list<conn_info> _conn_info_list;
		_conn_info_list.push_back(_conn_info);

		setConnInfoList(_conn_info_list);
		return true;
	}
};