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
#include "lbStrategy/lbsFactory.hpp"
#include "util/defs.hpp"
#include "serviceDiscovery/serviceDiscoveryFactory.hpp"
namespace connManager
{
const std::string CONN_INC = "CONN_INC";
const std::string CONN_DEC = "CONN_DEC";

// connmanager
template <typename DBConn>
class connManager : public gene::gene<void *>, public std::enable_shared_from_this<connManager<DBConn>>
{
public:
    connManager() = delete;
    connManager(std::shared_ptr<loop::loop> loopIn) : _loop(loopIn)
    {
    }
    virtual ~connManager()
    {
    }
#if 0
    void set_genetic_gene(void *gene)
    {
        _gene = gene;
    }
    void *get_genetic_gene()
    {
        return _gene;
    }
#endif
    bool init()
    {
        // load balance related
        _lbs_sptr = lbStrategy::lbsFactory<redisAsyncContext *>::create("RR");
        _lbs_sptr->init();
        // service discovery related
        std::shared_ptr<serviceDiscovery::serviceDiscovery<DBConn>> _srvc_sptr = serviceDiscovery::serviceDiscoveryFactory<DBConn>::create(getLoop());
        _srvc_sptr->init();
        _srvc_sptr->setOnConnInc(std::bind(&connManager<DBConn>::add_conn, this->get_shared_from_this()));
        _srvc_sptr->setOnConnDec(std::bind(&connManager<DBConn>::del_conn, this->get_shared_from_this()));

#if 0
        // message bus related
        auto bus = messageBusHelper<DBConn>(get_genetic_gene()).getMessageBus();
    
        auto self_sptr = std::get_shared_from_this();
        bus->register_handler(CONN_DEC, [self_sptr](DBConn obj) {
            self_sptr->getLbs()->del_obj(obj);
        });
        bus->register_handler(CONN_INC, [self_sptr](DBConn obj, unsigned int priority) {
            self_sptr->getLbs()->add_obj(obj, priority);
        });
#endif
    }

    void on_unavaliable() {}
    void on_avaliable() {}

    std::pair<DBConn, lbStrategy::retStatus> get_conn()
    {
        return _lbs_sptr.get_obj();
    }

    bool add_conn(dbw::CONN_INFO connInfo)
    {
        auto task_sptr = anySaver::anySaver<void *>::instance()->getData(get_genetic_gene, ANY_SAVER_TASK);
        task_sptr->send2task(task::taskMsgType::TASK_REDIS_ADD_CONN, connInfo);
        return true;
    }
    bool del_conn(dbw::CONN_INFO connInfo)
    {
    }

    std::shared_ptr<lbStrategy::lbStrategy<DBConn>> getLbs()
    {
        return _lbs_sptr;
    }
    std::shared_ptr<serviceDiscovery::serviceDiscovery<dbw::CONN_INFO>> getSds()
    {
        return _srvc_sptr;
    }
    std::shared_ptr<loop::loop> getLoop()
    {
        return _loop.lock();
    }

private:
    void *_gene;
    std::weak_ptr<loop::loop> _loop;
    std::shared_ptr<lbStrategy::lbStrategy<DBConn>> _lbs_sptr;
    std::shared_ptr<serviceDiscovery::serviceDiscovery<dbw::CONN_INFO>> _srvc_sptr;
};
} // namespace connManager