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
#include "lbStrategy/lbsFactory.hpp"
#include "util/defs.hpp"
#include "serviceDiscovery/serviceDiscoveryFactory.hpp"
#include <memory>

namespace connManager
{

template <typename DBConn>
class connManager : public gene::gene<void *>, public std::enable_shared_from_this<connManager<DBConn>>
{
public:
    using connChange = std::function<bool(std::shared_ptr<DBConn> connInfo)>;
    connManager() = delete;
    explicit connManager(std::shared_ptr<loop::loop> loopIn) : _loop(loopIn)
    {
    }
    virtual ~connManager()
    {
    }

    bool init()
    {
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "[connManager] init is called");
        }
        std::weak_ptr<connManager<DBConn>> self_wptr = getThisWptr();

        std::string lbName = configCenter::configCenter<void *>::instance()->getPropertiesField(getGeneticGene(), PROP_LOAD_BALANCE_STRATEGY, DEFAULT_LOAD_BALANCE_STRATEGY);
        // load balance related
        _lbs_sptr = lbStrategy::lbsFactory<redisAsyncContext *>::create(lbName);
        if (!_lbs_sptr)
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, " create load balancer fail!");
            }
            return false;
        }
        _lbs_sptr->init();

        _lbs_sptr->setFirstAvaliableCb([self_wptr]() {
            if (!self_wptr.expired())
            {
                self_wptr.lock()->onAvaliable();
            }
            else
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, " avaliable callback : connManager wptr is expired");
                }
            }
        });
        _lbs_sptr->setNoAvaliableCb([self_wptr]() {
            if (CHECK_LOG_LEVEL(warn))
            {
                __LOG(warn, "there is no avaliable connection");
            }
            if (!self_wptr.expired())
            {
                self_wptr.lock()->onUnavaliable();
            }
            else
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, " no avaliable callback : connManager wptr is expired");
                }
            }
        });
        // service discovery related
        std::string sdsName = configCenter::configCenter<void *>::instance()->getPropertiesField(getGeneticGene(), PROP_SERVICE_DISCOVERY_MODE, DEFAULT_SERVICE_DISCOVERY_MODE);

        _srvc_sptr = serviceDiscovery::serviceDiscoveryFactory<DBConn>::create(getLoop(), sdsName, getGeneticGene());

        _srvc_sptr->setOnConnInc([self_wptr](std::shared_ptr<DBConn> connInfo) {
            if (!self_wptr.expired())
            {
                if (CHECK_LOG_LEVEL(debug))
                {
                    __LOG(debug, "now there is a new connection, add it to connection manager");
                }
                return self_wptr.lock()->addConn(connInfo);
            }
            else
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "on connect inc : service discovery is expired");
                }
                return false;
            }
        });
        _srvc_sptr->setOnConnDec([self_wptr](std::shared_ptr<DBConn> connInfo) {
            if (!self_wptr.expired())
            {
                if (CHECK_LOG_LEVEL(debug))
                {
                    __LOG(debug, "delete a connection");
                }
                return self_wptr.lock()->delConn(connInfo);
            }
            else
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "on connect dev : service discovery is expired");
                }
                return false;
            }
        });
        _srvc_sptr->init();

        return true;
    }

    void onUnavaliable()
    {
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "onUnavaliable");
        }
        if (_unavaliable_cb)
        {
            _unavaliable_cb();
        }
        // there is not avaliable connection
        // retriger service discovery
        _srvc_sptr->retriger();
    }
    void onAvaliable()
    {
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "onAvaliable");
        }
        if (_avaliable_cb)
        {
            _avaliable_cb();
        }
    }

    void setAvaliableCb(std::function<void()> const &cb)
    {
        _avaliable_cb = cb;
    }
    std::function<void()> getAvaliableCb()
    {
        return _avaliable_cb;
    }
    void setUnavaliableCb(std::function<void()> const &cb)
    {
        _unavaliable_cb = cb;
    }
    std::function<void()> getUnavaliableCb()
    {
        return _unavaliable_cb;
    }

    DBW_OPT<redisAsyncContext *> get_conn()
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
    bool addConn(std::shared_ptr<DBConn> connInfo)
    {
        return connInc(connInfo);
    }
    bool delConn(std::shared_ptr<DBConn> connInfo)
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
    std::weak_ptr<connManager<DBConn>> getThisWptr()
    {
        std::weak_ptr<connManager<DBConn>> self_wptr(this->shared_from_this());
        return self_wptr;
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