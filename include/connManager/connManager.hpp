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
//#include "task/task.hpp"

#include <memory>

namespace connManager
{
const std::string CONN_INC = "CONN_INC";
const std::string CONN_DEC = "CONN_DEC";

// connmanager
template <typename DBConn>
class connManager : public gene::gene<void *>, public std::enable_shared_from_this<connManager<DBConn>>
{
public:
    using connChange = std::function<bool(DBConn connInfo)>;
    connManager() = delete;
    explicit connManager(std::shared_ptr<loop::loop> loopIn) : _loop(loopIn)
    {
    }
    virtual ~connManager()
    {
    }

    bool init()
    {
        __LOG(debug, "[connManager] init is called");
        auto this_sptr = this->shared_from_this();
        // load balance related
        _lbs_sptr = lbStrategy::lbsFactory<redisAsyncContext *>::create("RR");
        _lbs_sptr->init();

        _lbs_sptr->setFirstAvaliableCb([this_sptr]() {
            this_sptr->onAvaliable();
        });
        _lbs_sptr->setNoAvaliableCb([this_sptr]() {
            this_sptr->onUnavaliable();
        });
        // service discovery related
        std::string sdsName = configCenter::configCenter<void *>::instance()->getPropertiesField(getGeneticGene(), PROP_SERVICE_DISCOVERY_MODE, DEFAULT_SERVICE_DISCOVERY_MODE);

        _srvc_sptr = serviceDiscovery::serviceDiscoveryFactory<DBConn>::create(getLoop(), sdsName, getGeneticGene());

        _srvc_sptr->setOnConnInc([this_sptr](DBConn connInfo) {
            __LOG(debug, "new there is a new connection, add it to connection manager");
            return this_sptr->addConn(connInfo);
        });
        _srvc_sptr->setOnConnDec([this_sptr](DBConn connInfo) {
            __LOG(debug, "delete a connection");
            return this_sptr->delConn(connInfo);
        });
        _srvc_sptr->init();

        return true;
    }

    void onUnavaliable()
    {
        __LOG(debug, "onUnavaliable");
        if (_unavaliable_cb)
        {
            _unavaliable_cb();
        }
    }
    void onAvaliable()
    {
        __LOG(debug, "onAvaliable");
        if (_avaliable_cb)
        {
            _avaliable_cb();
        }
    }

    void setAvaliableCb(std::function<void()> cb)
    {
        _avaliable_cb = cb;
    }
    std::function<void()> getAvaliableCb()
    {
        return _avaliable_cb;
    }
    void setUnavaliableCb(std::function<void()> cb)
    {
        _unavaliable_cb = cb;
    }
    std::function<void()> getUnavaliableCb()
    {
        return _unavaliable_cb;
    }

    std::pair<redisAsyncContext *, lbStrategy::retStatus> get_conn()
    {
        return _lbs_sptr->getObj();
    }
    void setAddConnCb(connChange inc)
    {
        connInc = inc;
    }
    void setDecConnCb(connChange dec)
    {
        connDec = dec;
    }
    bool addConn(DBConn connInfo)
    {
        return connInc(connInfo);
    }
    bool delConn(DBConn connInfo)
    {
        return connDec(connInfo);
    }

    std::shared_ptr<lbStrategy::lbStrategy<redisAsyncContext *>> getLbs()
    {
        return _lbs_sptr;
    }
    std::shared_ptr<serviceDiscovery::serviceDiscovery<DBConn>> getSds()
    {
        return _srvc_sptr;
    }
    std::shared_ptr<loop::loop> getLoop()
    {
        return _loop.lock();
    }

private:

    std::weak_ptr<loop::loop> _loop;
    std::shared_ptr<lbStrategy::lbStrategy<redisAsyncContext *>> _lbs_sptr;
    std::shared_ptr<serviceDiscovery::serviceDiscovery<DBConn>> _srvc_sptr;
    connChange connInc;
    connChange connDec;
    std::function<void()> _avaliable_cb;
    std::function<void()> _unavaliable_cb;
};
} // namespace connManager