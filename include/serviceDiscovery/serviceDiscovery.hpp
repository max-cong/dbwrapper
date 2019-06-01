/*
 * Copyright (c) 2016-20019 Max Cong <savagecm@qq.com>
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
#include "util/nonCopyable.hpp"
namespace serviceDiscovery
{
// note: this is not thread safe. need to work with task
template <typename connInfo>
class serviceDiscovery : public gene::gene<void *>, public std::enable_shared_from_this<serviceDiscovery<connInfo>>, public nonCopyable
{
public:
    typedef std::list<std::shared_ptr<connInfo>> connList;

    typedef std::function<connList()> getConnFun;
    using onConnInfoChangeCb = std::function<bool(std::shared_ptr<connInfo>)>;

    serviceDiscovery<connInfo>() = delete;

    explicit serviceDiscovery<connInfo>(std::shared_ptr<loop::loop> loopIn)
    {
        _timerManager.reset(new timer::timerManager(loopIn));
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "[serviceDiscovery] serviceDiscovery");
        }
        _retrigerTimer = _timerManager->getTimer();
    }

    virtual bool init() = 0;

    virtual bool stop()
    {
        __LOG(warn, "[serviceDiscovery] stop is called");
        _retrigerTimer->stop();
        for (auto it : _conn_list)
        {
            __LOG(warn, "[service Discovery] : now delete ip : " << it->ip << ", port : " << it->port);
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
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "[serviceDiscovery] [retriger]");
        }
        if (!(_retrigerTimer->getIsRunning()))
        {
            std::string reccItval = configCenter::configCenter<void *>::instance()->getPropertiesField(getGeneticGene(), PROP_SD_RETRIGER_INTERVAL, DEFAULT_SD_RETRIGER_INTERVAL);
            std::string::size_type sz; // alias of size_t
            int _reconnect_interval = std::stoi(reccItval, &sz) * 1000;
            auto self_wptr = getThisWptr();
            if (CHECK_LOG_LEVEL(debug))
            {
                __LOG(debug, "service discovery retriger timer is finished, start a new timer [" << _reconnect_interval << "ms]");
            }
            _retrigerTimer->startOnce(_reconnect_interval, [self_wptr]() {
                if (!self_wptr.expired())
                {
                    __LOG(warn, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!restart service discovery!!!!!!!!!!!!!!!!!!!!!!!!!");
                    self_wptr.lock()->restart();
                }
                else
                {
                    __LOG(warn, "service discovery : this wptr is expired!");
                }
            });
        }
        else
        {
            if (CHECK_LOG_LEVEL(debug))
            {
                __LOG(debug, "retriger timer is running");
            }
        }
        return true;
    }
    virtual void onConnInfoInc(std::shared_ptr<connInfo> info)
    {
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "[serviceDiscovery] onConnInfoInc");
        }
        if (_cfgInc)
        {
            if (CHECK_LOG_LEVEL(debug))
            {
                __LOG(debug, "[serviceDiscovery] call registered callback function");
            }
            _cfgInc(info);
        }
    }
    virtual void onConnInfoDec(std::shared_ptr<connInfo> info)
    {
        if (_cfgDec)
        {
            _cfgDec(info);
        }
    }
    virtual void setOnConnInc(onConnInfoChangeCb cfgIncCb)
    {
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "[serviceDiscovery] set conn inc callback function");
        }
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
    virtual connList getConnInfoList()
    {
        return _conn_list;
    }
    virtual void deleteConnInfo(connList _list)
    {

        for (auto it : _list)
        {
            _conn_list.remove_if([&it, this](std::shared_ptr<connInfo> _info) {
                if (*it == *_info)
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

        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "now try to get connection info ");
        }

        if (update_list.empty())
        {
            if (CHECK_LOG_LEVEL(debug))
            {
                __LOG(debug, "update_list is empty, now just clear the _conn_list");
            }
            _conn_list.clear();
            return true;
        }
        // in the update lost but not in the conn list
        // note: the list contains shared pointer.
        // the operator "==" means shared_ptr.get() contains the same pointer
        for (auto tmp : update_list)
        {
            if (std::find(std::begin(_conn_list), std::end(_conn_list), tmp) == _conn_list.end())
            {
                onConnInfoInc(tmp);
                _conn_list.push_back(tmp);
                if (CHECK_LOG_LEVEL(debug))
                {
                    __LOG(debug, "[service_discovery_base] now there is a new connection.");
                }
            }
            else
            {
                if (CHECK_LOG_LEVEL(debug))
                {
                    __LOG(debug, "[service_discovery_base] connection info already in the local list");
                }
            }
        }
        // in the connection list but not in the update list , to remove
        connList tmplist_not_in_tmp_list;
        for (auto tmp : _conn_list)
        {
            if (std::find(std::begin(update_list), std::end(update_list), tmp) == update_list.end())
            {
                onConnInfoDec(tmp);
                tmplist_not_in_tmp_list.push_back(tmp);
            }
            else
            {
                if (CHECK_LOG_LEVEL(debug))
                {
                    __LOG(debug, "[service_discovery_base] connection info already in the local list");
                }
            }
        }

        for (auto to_rm : tmplist_not_in_tmp_list)
        {
            std::remove(_conn_list.begin(), _conn_list.end(), to_rm);
        }

        return true;
    }

    std::weak_ptr<serviceDiscovery<connInfo>> getThisWptr()
    {
        std::weak_ptr<serviceDiscovery<connInfo>> self_wptr(this->shared_from_this());
        return self_wptr;
    }

    connList _conn_list;

    timer::timer::ptr_p _retrigerTimer;
    std::shared_ptr<timer::timerManager> _timerManager;
    onConnInfoChangeCb _cfgInc;
    onConnInfoChangeCb _cfgDec;
};
} // namespace serviceDiscovery
