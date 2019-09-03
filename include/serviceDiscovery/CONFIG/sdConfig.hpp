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
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "get host : " << connHost << ", port is : " << connPort);
        }
        // build connInfo
        std::shared_ptr<connInfo> _connInfo_sptr = std::make_shared<connInfo>();

        _connInfo_sptr->type = medis::CONN_TYPE::IP;
        _connInfo_sptr->ip = connHost;

        std::string::size_type sz; // alias of size_t

        int port = std::stoi(connPort, &sz);

        _connInfo_sptr->port = port;
        std::list<std::shared_ptr<connInfo>> _connInfo_list;
        _connInfo_list.push_back(_connInfo_sptr);

        return this->updateConnInfo(_connInfo_list);
    }
};
} // namespace serviceDiscovery
