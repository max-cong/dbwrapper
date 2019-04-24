#pragma once

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
#include "timer/timerManager.hpp"
#include "util/nonCopyable.hpp"

#include "boost/asio/ip/address.hpp"
#include "boost/any.hpp"
#include <boost/algorithm/string.hpp>
#include <typeinfo>
#include <list>
namespace serviceDiscovery
{

using List = std::list<std::string>;
template <typename connInfo>
class sdDns : public serviceDiscovery<connInfo>
{
public:
    sdDns<connInfo>() = delete;
    explicit sdDns<connInfo>(std::shared_ptr<loop::loop> loopIn) : serviceDiscovery<connInfo>(loopIn)
    {
        _dns_timer = this->_timerManager->getTimer();
        _dns_ttl = 0;
    }


    virtual bool retriger() override
    {
        __LOG(debug, "[dns retriger]");
        _dns_timer->stop();
        serviceDiscovery<connInfo>::retriger();
        return true;
    }

    // this is called by APP thread, lock is for this(setConnInfoList have lock)
    void dnsResolveCallback(List &ipList, int dns_ttl)
    {
// to do : need to add task to anysaver
#if 0
        __LOG(debug, "[sdDns] dnsResolveCallback ");
        {
            __LOG(debug, "IP list size is : " << ipList.size() << ", list is :");
            for (auto it : ipList)
            {
                __LOG(debug, "-- " << it << " --");
            }
        }
        if (dns_ttl)
        {
            _dns_ttl = dns_ttl;
        }
        else
        {
            _dns_ttl = 5;
        }
        connList _tmp_list;
        for (auto _tmp_host : ipList)
        {
            auto ret = host2connInfo(_tmp_host);
            std::string _port = config_center<void *>::instance()->get_properties_fields(get_genetic_gene(), PROP_PORT, DEFAULT_PORT_STR);
            ret.first.port = _port;

            unsigned int dscp = config_center<void *>::instance()->get_properties_fields(get_genetic_gene(), PROP_DSCP, DEFAULT_DSCP);
            ret.first.dscp = dscp;

            if (ret.second)
            {
                _tmp_list.push_back(ret.first);
            }
            else
            {
                __LOG(warn, "invalid host!");
            }
        }
        if (!_tmp_list.empty())
        {
            setConnInfoList(_tmp_list);
        }
#endif
    }
#if 0
    bool do_dns()
    {
        
        // if er run into here, that means the PROP_HOST in the configration is FQDN
        // get FQND from configuration, is FQDN is not empty, then call DNS related function
        std::string _host = config_center<void *>::instance()->get_properties_fields(get_genetic_gene(), PROP_HOST, DEFAULT_HOST);

        // call DNS resolve callback and call it
        auto ret = any_saver<void *>::instance()->get_data(get_genetic_gene(), DNS_RESOLVE_FQDN);
        if (ret.second)
        {
            try
            {
                auto _cb = boost::any_cast<boost::function<void(std::string)>>(ret.first);
                _cb(_host);
                return true;
            }
            catch (boost::bad_any_cast &)
            {
                __LOG(error, "cast error");
                return false;
            }
        }
        else
        {
            __LOG(error, "there is no DNS callback function in the any saver!");
            return false;
        }
    }
    // do dns and reload the timer
    // note: app thread will change _dns_ttl
    // timer thread will reload according to _dns_ttl
    void do_dns_with_timer()
    {
        do_dns();
        std::string _refresh = config_center<void *>::instance()->get_properties_fields(get_genetic_gene(), PROP_DNS_REFRESHMENT_IND, DEFAULT_DNS_REFRESH_IND);
        if (!_refresh.compare("False"))
        {
            if (_dns_timer)
            {
                _dns_timer->startOnce(_dns_ttl * 1000, std::bind(&sdDns::do_dns_with_timer, this));
            }
            else
            {
                __LOG(error, "timer is not valid!!");
            }
        }
        else
        {
            __LOG(debug, "do not need to refresh!");
        }
    }
#endif
    // note: init may return fail. then do not start the system and check the configuration of host!
    virtual bool init() override
    {
#if 0
        __LOG(debug, "[sdDns] init is called");
        boost::function<bool()> _cfg_change_fn = boost::bind(&sdDns::stop, this);
        // register callback to any saver
        auto cfg_change_ret = any_saver<void *>::instance()->save_data(get_genetic_gene(), CFG_CHANGE_SERVICE_DISCOVERY_MESSAGE, _cfg_change_fn);

        if (!cfg_change_ret)
        {
            return false;
        }

        __LOG(debug, "[sdDns] sdDns init");
        unsigned int _default_ttl = config_center<void *>::instance()->get_properties_fields(get_genetic_gene(), PROP_TTL_VALUE, DEFAULT_TTL_VALUE);
        _dns_ttl = _default_ttl;
        boost::function<void(List &, int)> _dns_resolve_fn = boost::bind(&sdDns::dnsResolveCallback, this, _1, _2);

        // register callback to any saver
        auto ret = any_saver<void *>::instance()->save_data(get_genetic_gene(), DNS_RESP_IP_LIST, _dns_resolve_fn);
        if (!ret)
        {
            return false;
        }

        // get FQND from configuration, if FQDN is not empty, then call DNS related function
        std::string _host = config_center<void *>::instance()->get_properties_fields(get_genetic_gene(), PROP_HOST, DEFAULT_HOST);
        __LOG(debug, "get host : " << _host);

        std::string _port = config_center<void *>::instance()->get_properties_fields(get_genetic_gene(), PROP_PORT, DEFAULT_PORT_STR);

        // get host list
        List hosts;

        boost::algorithm::split(hosts, _host, boost::is_any_of(","));

        {
            __LOG(debug, "host list size is : " << hosts.size());
            for (auto it : hosts)
            {
                __LOG(debug, "-- " << it << " --");
            }
        }

        // if IPv4/IPv6 or FQDN, If FQDN appear, then use DNS
        connList _tmp_list;
        for (auto _host_iter : hosts)
        {
            try
            {
                boost::asio::ip::address addr(boost::asio::ip::address::from_string(_host_iter));
                connInfo _tmp_info;
                if (addr.is_v4())
                {
                    __LOG(debug, "the host in configuration is IPv4");
                    _tmp_info.type = conn_type::IPv4;
                    _tmp_info.ip = _host_iter;
                    _tmp_info.port = _port;
                    _tmp_list.push_back(_tmp_info);
                }
                else if (addr.is_v6())
                {
                    __LOG(debug, "the host in configuration is IPv6");
                    _tmp_info.type = conn_type::IPv6;
                    _tmp_info.ip = _host_iter;
                    _tmp_info.port = _port;
                    _tmp_list.push_back(_tmp_info);
                }
                else
                {
                    __LOG(error, "the host in configuration is not known");
#if 0
                    do_dns_with_timer();
                    return true;
#endif
                    return false;
                }
            }
            catch (boost::system::system_error &e)
            {
                __LOG(debug, "convert FQDN got an exception! this must be FQDN");
                do_dns_with_timer();
                return true;
                // return false;
            }
        }
        // do not need to do FQDN, just use the IP
        if (!_tmp_list.empty())
        {
            setConnInfoList(_tmp_list);
        }
        else
        {
            __LOG(error, "the config is not FQDN, but there is no IP!");
        }

        return true;
#endif
        return true;
    }

    // refresh interval
    int _dns_ttl;
    timer::timer::ptr_p _dns_timer;
};
} // namespace serviceDiscovery