/*
 * Copyright (c) 2016-20017 Max Cong <savagecm@qq.com>
 * this code can be found at https://github.com/maxcong001/connection_manager
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
#include <list>
#include <unordered_set>
#include <algorithm>
#include "translib/timerManager.h"
#include "translib/timer.h"
#include "logger/logger.hpp"
#include "service_discovery/util.hpp"

template <typename connInfo>
class service_discovery : public genetic_gene_void_p
{
  public:
    typedef std::list<connInfo> connList;
    typedef std::function<connList()> getConnFun;
    typedef std::function<bool(connInfo)> onConnInfoChangeCb;
    typedef connInfo connInfo_type_t;

    service_discovery<connInfo>(std::shared_ptr<translib::TimerManager> timer_manager = nullptr)
    {
        __LOG(debug, "[service_discovery] service_discovery");
        _timer_manager = timer_manager;
        _retriger_timer = timer_manager->getTimer();
    }

    virtual ~service_discovery<connInfo>()
    {
        _retriger_timer->stop();
        __LOG(warn, "[service_discovery] ~service_discovery");
    }
    virtual bool init() = 0;

    std::pair<conn_info, bool> host2conn_info(std::string host)
    {
        conn_info _tmp_info;
        try
        {
            boost::asio::ip::address addr(boost::asio::ip::address::from_string(host));
            if (addr.is_v4())
            {
                _tmp_info.type = conn_type::IPv4;
                _tmp_info.ip = host;
                return std::make_pair(_tmp_info, true);
            }
            else if (addr.is_v6())
            {
                _tmp_info.type = conn_type::IPv6;
                _tmp_info.ip = host;
                return std::make_pair(_tmp_info, true);
            }
            else
            {
                return std::make_pair(_tmp_info, false);
            }
        }
        catch (boost::system::system_error &e)
        {
            return std::make_pair(_tmp_info, false);
        }
    }

    virtual bool stop()
    {
        __LOG(warn, "[service_discovery] stop is called");
        for (auto it : _conn_list)
        {
            onConnInfoDec(it);
        }
        _conn_list.clear();
        //_tmp_conn_list.clear();
        return true;
    }

    // restart with init function, note : this will triger service discovery
    virtual bool restart()
    {
        std::lock_guard<std::recursive_mutex> lck(_mutex);
        __LOG(warn, "[service_discovery] restart is called");
        stop();
        return init();
    }
    virtual bool retriger()
    {
        __LOG(debug, "[retriger]");
        if (!(_retriger_timer->get_is_running()))
        {
            __LOG(debug, "retriger timer is finished, start a new timer");
            int _reconnect_interval = config_center<void *>::instance()->get_properties_fields(get_genetic_gene(), PROP_RECONN_INTERVAL, DEFAULT_CONN_TIMER);
            _retriger_timer->startOnce(_reconnect_interval, [this]() {
                __LOG(error, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!restart!!!!!!!!!!!!!!!!!!!!!!!!!");
                this->restart();
            });
        }
        else
        {
            __LOG(debug, "retriger timer is running");
        }
        return true;
    }

    virtual void onConnInfoInc(connInfo info)
    {
        if (_cfgInc)
        {
            _cfgInc(info);
        }
    }
    virtual void onConnInfoDec(connInfo info)
    {
        if (_cfgDec)
        {
            _cfgDec(info);
        }
    }
    virtual void setOnConnInc(onConnInfoChangeCb cfgIncCb)
    {
        _cfgInc = cfgIncCb;
    }
    virtual void setOnConnDec(onConnInfoChangeCb cfgDecCb)
    {
        _cfgDec = cfgDecCb;
    }

    virtual void setConnInfoList(connList _list)
    {

        //std::lock_guard<std::recursive_mutex> lck(_mutex);
        //_tmp_conn_list.swap(_list);

        updateConnInfo(_list);
    }
    virtual void deleteConnInfo(connList _list)
    {
        std::lock_guard<std::recursive_mutex> lck(_mutex);
        for (auto it : _list)
        {
            _conn_list.remove_if([&it, this](conn_info _info) {
                if (it == _info)
                {
                    onConnInfoDec(_info);
                    return true;
                }
                else
                {
                    return false;
                }
            });
        }
    }

    virtual bool updateConnInfo(connList update_list)
    {
        std::lock_guard<std::recursive_mutex> lck(_mutex);
        __LOG(debug, "now try to get connection info ");
        {
            __LOG(debug, "dump update_list: ")
            for (auto it : update_list)
            {
                __LOG(debug, "-- " << it.ip << " -- " << it.port << " --");
            }
        }

        if (update_list.empty())
        {
            __LOG(debug, "update_list is empty, now just clear the _conn_list");
            _conn_list.clear();
            return true;
        }

        for (auto tmp : update_list)
        {
            if (std::find(std::begin(_conn_list), std::end(_conn_list), tmp) == _conn_list.end())
            // if (_conn_list.find(tmp) == _conn_list.end())
            {
                onConnInfoInc(tmp);
                _conn_list.push_back(tmp);
                __LOG(debug, "[service_discovery_base] now there is a new connection.");
                //tmp.dump();
            }
            else
            {
                __LOG(debug, "[service_discovery_base] connection info already in the local list");
                //tmp.dump();
            }
        }

        connList tmplist_not_in_tmp_list;
        for (auto tmp : _conn_list)
        {
            if (std::find(std::begin(update_list), std::end(update_list), tmp) == update_list.end())
            //if (update_list.find(tmp) == update_list.end())
            {
                onConnInfoDec(tmp);
                __LOG(warn, "[service_discovery_base] now there is a connection to delete. info : " << tmp.ip);
                tmplist_not_in_tmp_list.push_back(tmp);
            }
            else
            {
            }
        }

        for (auto to_rm : tmplist_not_in_tmp_list)
        {

            std::remove(_conn_list.begin(), _conn_list.end(), to_rm);
            __LOG(warn, "remove conn : " << to_rm.ip << ", now there are : " << _conn_list.size() << " connection");
        }

        return true;
    }

    connList _conn_list;
    //connList _tmp_conn_list;

    translib::Timer::ptr_p _retriger_timer;
    std::shared_ptr<translib::TimerManager> _timer_manager;
    onConnInfoChangeCb _cfgInc;
    onConnInfoChangeCb _cfgDec;

    std::recursive_mutex _mutex;
};