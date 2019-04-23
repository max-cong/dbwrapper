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

#include "logger/logger.hpp"
#include "loop/loop.hpp"
#include "gene/gene.hpp"
#include "configCenter/configCenter.hpp"
namespace serviceDiscovery
{
// note: this is not thread safe. need to work with task
template <typename connInfo>
class serviceDiscovery : public gene::gene<void *>
{
public:
    typedef std::list<connInfo> connList;
    typedef std::function<connList()> getConnFun;
    typedef std::function<bool(connInfo)> onConnInfoChangeCb;
    typedef connInfo connInfo_type_t;
    serviceDiscovery<connInfo>() = delete;
    serviceDiscovery<connInfo>(std::shared_ptr<loop::loop> loopIn)
    {
        _timerManager.reset(new timer::timerManager(loopIn));
        __LOG(debug, "[serviceDiscovery] serviceDiscovery");
        _retrigerTimer = _timerManager->getTimer();
    }

    virtual ~serviceDiscovery<connInfo>()
    {
        _retrigerTimer->stop();
        __LOG(warn, "[serviceDiscovery] ~serviceDiscovery");
    }
    virtual bool init() = 0;
#if 0
    std::pair<connInfo, bool> host2connInfo(std::string host)
    {
        connInfo _tmp_info;
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
#endif
    virtual bool stop()
    {
        __LOG(warn, "[serviceDiscovery] stop is called");
        for (auto it : _conn_list)
        {
            onConnInfoDec(it);
        }
        _conn_list.clear();
        return true;
    }

    // restart with init function, note : this will triger service discovery
    virtual bool restart()
    {
        __LOG(warn, "[serviceDiscovery] restart is called");
        stop();
        return init();
    }
    virtual bool retriger()
    {
        __LOG(debug, "[serviceDiscovery] [retriger]");
        if (!(_retrigerTimer->getIsRunning()))
        {
            __LOG(debug, "retriger timer is finished, start a new timer");

            std::string reccItval = configCenter::configCenter<void *>::instance()->get_properties_fields(get_genetic_gene(), PROP_RECONN_INTERVAL, DEFAULT_RECONN_INTERVAL);
            std::string::size_type sz; // alias of size_t
            int _reconnect_interval = std::stoi(reccItval, &sz);

            _retrigerTimer->startOnce(_reconnect_interval, [this]() {
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

        updateConnInfo(_list);
    }
    virtual void deleteConnInfo(connList _list)
    {

        for (auto it : _list)
        {
            _conn_list.remove_if([&it, this](connInfo _info) {
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

        __LOG(debug, "now try to get connection info ");

        __LOG(debug, "dump update_list: ")
        for (auto it : update_list)
        {
            __LOG(debug, "-- " << it.ip << " -- " << it.port << " --");
        }

        if (update_list.empty())
        {
            __LOG(debug, "update_list is empty, now just clear the _conn_list");
            _conn_list.clear();
            return true;
        }
        // in the update lost but not in the conn list
        for (auto tmp : update_list)
        {
            if (std::find(std::begin(_conn_list), std::end(_conn_list), tmp) == _conn_list.end())
            {
                onConnInfoInc(tmp);
                _conn_list.push_back(tmp);
                __LOG(debug, "[service_discovery_base] now there is a new connection.");
            }
            else
            {
                __LOG(debug, "[service_discovery_base] connection info already in the local list");
            }
        }
        // in the connection list but not in the update list , to remove
        connList tmplist_not_in_tmp_list;
        for (auto tmp : _conn_list)
        {
            if (std::find(std::begin(update_list), std::end(update_list), tmp) == update_list.end())
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

    timer::timer::ptr_p _retrigerTimer;
    std::shared_ptr<timer::timerManager> _timerManager;
    onConnInfoChangeCb _cfgInc;
    onConnInfoChangeCb _cfgDec;
};
} // namespace serviceDiscovery