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
#include "service_discovery.hpp"
#include "common/util.hpp"
#include "config/config_util.hpp"
#include "service_discovery/DNS/dns.hpp"
#include "service_discovery/ETCD/ETCD_INT/etcd_int.hpp"
#include "service_discovery/ETCD/ETCD_L4TD/etcd_l4td.hpp"
#include "service_discovery/UNIX_SOCKET/unix_socket.hpp"

using namespace dbw;
class service_discovery_factory : public genetic_gene_void_p
{
  public:
    service_discovery_factory(std::shared_ptr<translib::TimerManager> timer = nullptr){};
    virtual ~service_discovery_factory(){};

    static std::string get_mode(void *gen)
    {
       
        return name;
    }
    template <typename connInfo>
    static std::shared_ptr<service_discovery<connInfo>> create(std::shared_ptr<translib::TimerManager> timer, void *gen)
    {
        std::shared_ptr<service_discovery<connInfo>> ret = nullptr;
        std::string name = get_mode(gen);

        if (!name.compare("DNS"))
        {
            ret = std::make_shared<dns_service_discovery>(timer);
            if (ret)
            {
                ret->set_genetic_gene(gen);
            }
            else
            {
                __LOG(error, "create service discovery obj fail!");
            }
        }
 
        else if (!name.compare("unix_socket"))
        {
            ret = std::make_shared<unix_socket_service_discovery>(timer);
            if (ret)
            {
                ret->set_genetic_gene(gen);
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