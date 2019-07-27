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
#include <atomic>
#include <functional>
#include "timer/timerManager.hpp"
#include "logger/logger.hpp"
#include "gene/gene.hpp"
#include "loop/loop.hpp"
#include "configCenter/configCenter.hpp"
#include "configCenter/configUtil.hpp"
#include <memory>
#include <atomic>

namespace heartBeat
{
class heartBeat : public gene::gene<void *>, public std::enable_shared_from_this<heartBeat>
{
public:
    using ping_f = std::function<void()>;
    using hbSuccCb = std::function<void(void)>;
    using hbLostCb = std::function<void(void)>;
    heartBeat() = delete;
    explicit heartBeat(std::shared_ptr<loop::loop> loopIn) : _interval(3000), _loop(loopIn), _success(false), _retryNum(5)
    {
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "start heartBeat, this is :" << (void *)this);
        }
    }

    bool init()
    {
        std::string intervalStr = configCenter::configCenter<void *>::instance()->getPropertiesField(getGeneticGene(), PROP_HB_INTERVAL, DEFAULT_HB_INTERVAL);
        std::string::size_type sz; // alias of size_t
        _interval = std::stoi(intervalStr, &sz) * 1000;

        std::string hbLostNum = configCenter::configCenter<void *>::instance()->getPropertiesField(this_sptr->getGeneticGene(), PROP_HB_LOST_NUM, DEFAULT_HB_LOST_NUM);
        std::string::size_type sz; // alias of size_t
        int i_dec = std::stoi(hbLostNum, &sz);
        _retryNum = i_dec;

        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "start heartbeat with interval [" << _interval << "ms], "
                                                           << "retry " << _retryNum << " timrs.");
        }
        _tManager.reset(new timer::timerManager(_loop.lock()));
        start();
        return true;
    }
    void setRetryNum(unsigned int retryNum)
    {
        _retryNum = retryNum;
    }
    unsigned int getRetryNum()
    {
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "[getRetryNum] this is : " << (void *)this);
        }
        return _retryNum;
    }
    void onHeartbeatLost()
    {
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "onHeartbeatLost");
        }
        _hbTimer->stop();
        if (_hbLostCb)
        {
            _hbLostCb();
        }
    }
    void onHeartbeatSuccess()
    {
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "onHeartbeatSuccess");
        }
        if (_hbSuccCb)
        {
            _hbSuccCb();
        }
    }

    void start()
    {

        // first we need to regard the heart beat is success
        // As ping is not send
        // At this time we should not call the ping function
        // as the connection may not set up yet
        // the worest case is to detect APP not avaliable net heart beat
        setHbSuccess(false);
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "start heartBeat and send heartbeat!");
        }
        if (getPingCb())
        {
            getPingCb()();
        }

        _hbTimer = _tManager->getTimer();
        if (!_hbTimer)
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "[heartbeat] get _hbTimer fail");
            }
            return;
        }
        auto self_wptr = getThisWptr();

        _processHb = [self_wptr]() {
            if (self_wptr.expired())
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "heart beat timer: heat beat weak ptr is expired");
                }
                return;
            }
            auto this_sptr = self_wptr.lock();
            if (this_sptr->getHbSuccess())
            {
                this_sptr->onHeartbeatSuccess();

                std::string hbLostNum = configCenter::configCenter<void *>::instance()->getPropertiesField(this_sptr->getGeneticGene(), PROP_HB_LOST_NUM, DEFAULT_HB_LOST_NUM);
                std::string::size_type sz; // alias of size_t

                int i_dec = std::stoi(hbLostNum, &sz);
                this_sptr->_retryNum = i_dec;
            }
            else
            {

                this_sptr->_retryNum--;
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "heartbeat fail, retry time left : " << this_sptr->_retryNum << ", this is : " << (void *)this_sptr.get());
                }
                if (this_sptr->_retryNum < 1)
                {
                    this_sptr->_retryNum = 0;
                }
            }

            // set heartBeat status false, if hb success, it will set to true
            this_sptr->setHbSuccess(false);

            if (this_sptr->_retryNum < 1)
            {
                std::string hbLostNum = configCenter::configCenter<void *>::instance()->getPropertiesField(this_sptr->getGeneticGene(), PROP_HB_LOST_NUM, DEFAULT_HB_LOST_NUM);
                std::string::size_type sz; // alias of size_t

                int i_dec = std::stoi(hbLostNum, &sz);
                this_sptr->_retryNum = i_dec;

                this_sptr->onHeartbeatLost();
            }

            if (this_sptr->getPingCb())
            {
                (this_sptr->getPingCb())();
            }
            this_sptr->_hbTimer->startOnce(this_sptr->_interval, this_sptr->_processHb);
        };
        _hbTimer->startOnce(_interval, _processHb);
    }
    bool stop()
    {
        _tManager->stop();
        return true;
    }

    void setInterval(uint32_t iv) { _interval = iv; }
    uint32_t getInterval() { return _interval; }
    void setHbSuccCb(hbSuccCb const &cb) { _hbSuccCb = cb; }
    void setHbLostCb(hbLostCb const &cb) { _hbLostCb = cb; }
    void setPingCb(ping_f const &cb) { _pingCb = cb; }
    ping_f getPingCb() { return _pingCb; }

    void setHbSuccess(bool success)
    {
        _success = success;
    }
    bool getHbSuccess()
    {
        bool ret = _success;
        return ret;
    }
    std::weak_ptr<heartBeat> getThisWptr()
    {
        std::weak_ptr<heartBeat> self_wptr(shared_from_this());
        return self_wptr;
    }

private:
    uint32_t _interval;

    hbSuccCb _hbSuccCb;
    hbLostCb _hbLostCb;
    ping_f _pingCb;
    std::weak_ptr<loop::loop> _loop;
    std::atomic<bool> _success;
    std::atomic<unsigned int> _retryNum;
    std::shared_ptr<timer::timer> _hbTimer;
    std::shared_ptr<timer::timerManager> _tManager;
    std::function<void()> _processHb;
};
} // namespace heartBeat