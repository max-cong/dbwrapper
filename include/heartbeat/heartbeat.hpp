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
    using ping_f = std::function<void(std::shared_ptr<heartBeat>)>;
    using hbSuccCb = std::function<void(void)>;
    using hbLostCb = std::function<void(void)>;
    heartBeat() = delete;
    heartBeat(std::shared_ptr<loop::loop> loopIn) : _interval(5000), _loop(loopIn), _success(false), _retryNum(5)
    {
        __LOG(debug, "start heartBeat, this is :" << (void *)this);
    }
    ~heartBeat()
    {
        __LOG(debug, "[heartBeat] ~heartBeat, this is : " << (void *)this);
    }
    bool init()
    {
        _tManager.reset(new timer::timerManager(_loop.lock()));
        return true;
    }
    void onHeartbeatLost()
    {
        __LOG(debug, "onHeartbeatLost");
        if (_hbLostCb)
        {
            _hbLostCb();
        }
    }
    void onHeartbeatSuccess()
    {
        __LOG(debug, "onHeartbeatSuccess");
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
        set_hb_success(true);
        __LOG(debug, "start heartBeat!");

        auto timer = _tManager->getTimer();
        if (!timer)
        {
            __LOG(error, "[heartbeat] get timer fail");
            return;
        }
        auto this_sptr = shared_from_this();
        timer->startForever(_interval, [this_sptr]() {
            if (this_sptr->get_hb_success())
            {
                this_sptr->onHeartbeatSuccess();

                std::string hbLostNum = configCenter::configCenter<void *>::instance()->get_properties_fields(this_sptr->get_genetic_gene(), PROP_HB_LOST_NUM, DEFAULT_HB_LOST_NUM);
                std::string::size_type sz; // alias of size_t

                int i_dec = std::stoi(hbLostNum, &sz);
                this_sptr->_retryNum = i_dec;
            }
            else
            {
                this_sptr->_retryNum--;
                if (this_sptr->_retryNum < 1)
                {
                    this_sptr->_retryNum = 0;
                }
            }

            // set heartBeat status false, if hb success, it will set to true
            this_sptr->set_hb_success(false);

            if (this_sptr->_retryNum < 1)
            {

                std::string hbLostNum = configCenter::configCenter<void *>::instance()->get_properties_fields(this_sptr->get_genetic_gene(), PROP_HB_LOST_NUM, DEFAULT_HB_LOST_NUM);
                std::string::size_type sz; // alias of size_t

                int i_dec = std::stoi(hbLostNum, &sz);
                this_sptr->_retryNum = i_dec;

                this_sptr->onHeartbeatLost();
            }

            if (this_sptr->getPingCb())
            {
                (this_sptr->getPingCb())(this_sptr);
            }
        });
    }
    bool stop()
    {
        _tManager->stop();
        return true;
    }

    void setInterval(uint32_t iv) { _interval = iv; }
    uint32_t getInterval() { return _interval; }
    void setHbSuccCb(hbSuccCb cb) { _hbSuccCb = cb; }
    void setHbLostCb(hbLostCb cb) { _hbLostCb = cb; }
    void setPingCb(ping_f cb) { _pingCb = cb; }
    ping_f getPingCb() { return _pingCb; }

    void set_hb_success(bool success)
    {
        _success = success;
    }
    bool get_hb_success()
    {
        bool ret = _success;
        return ret;
    }

private:
    uint32_t _interval;

    hbSuccCb _hbSuccCb;
    hbLostCb _hbLostCb;
    ping_f _pingCb;
    std::weak_ptr<loop::loop> _loop;
    std::atomic<bool> _success;
    std::atomic<unsigned int> _retryNum;
    std::shared_ptr<timer::timerManager> _tManager;
};
} // namespace heartBeat