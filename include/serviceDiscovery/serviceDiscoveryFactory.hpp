#pragma once
/*
 * Copyright (c) 2016-20017 Max Cong <savagecm@qq.com>
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
#include <string>
#include <memory>
#include "gene/gene.hpp"
#include "serviceDiscovery/serviceDiscovery.hpp"
#include "serviceDiscovery/DNS/sdDns.hpp"
#include "serviceDiscovery/UNIX_SOCKET/sdUnixSocket.hpp"
#include "loop/loop.hpp"
#include "configCenter/configCenter.hpp"
namespace serviceDiscovery
{
class serviceDiscoveryFactory : public gene::gene<void *>
{
public:
    serviceDiscoveryFactory(){};
    virtual ~serviceDiscoveryFactory(){};

    static std::string get_mode()
    {
        std::string name = configCenter::configCenter<void *>::instance()->get_properties_fields(get_genetic_gene(), configCenter::PROP_SERVICE_DISCOVERY_MODE, configCenter::DEFAULT_SERVICE_DISCOVERY_MODE);
        return name;
    }
    template <typename connInfo>
    static std::shared_ptr<serviceDiscovery<connInfo>> create(std::shared_ptr<loop::loop> loopIn)
    {
        std::shared_ptr<serviceDiscovery<connInfo>> ret = nullptr;
        std::string name = get_mode();

        if (!name.compare("DNS"))
        {
            ret = std::make_shared<sdDns>(loopIn);
            if (ret)
            {
                ret->set_genetic_gene(get_genetic_gene());
            }
            else
            {
                __LOG(error, "create service discovery obj fail!");
            }
        }
        else if (!name.compare("unix_socket"))
        {
            ret = std::make_shared<sdUnixSocket>(loopIn);
            if (ret)
            {
                ret->set_genetic_gene(get_genetic_gene());
            }
            else
            {
                __LOG(error, "create service discovery obj fail!");
            }
        }
        else
        {
            __LOG(warn, "not support type!");
        }
        return ret;
    }
};
} // namespace serviceDiscovery