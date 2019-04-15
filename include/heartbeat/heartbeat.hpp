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
#include "translib/timerManager.h"
#include "logger/logger.hpp"

#include "configCenter/configCenter.hpp"

namespace heartBeat
{
class heartbeat : public gene::gene
{
  public:
    typedef std::function<void()> ping_f;
    typedef std::function<void(void)> hb_success_cb;
    typedef std::function<void(int)> hb_lost_cb;
    heartbeat(translib::Timer::ptr_p timer)
    {
        __LOG(debug, "start heartbeat, this is :" << (void *)this);
        _timer = timer;
        interval = 5000;
        _success = false;
        _num_to_reconnect = config_center<void *>::instance()->get_properties_fields(get_genetic_gene(), dbw::PROP_HB_LOST_NUM, dbw::DEFAULT_HB_LOST_NUM);
    }
    ~heartbeat()
    {
        __LOG(debug, "[heartbeat] ~heartbeat, this is : " << (void *)this);
        _timer->stop();
    }
    void onHeartbeatLost()
    {
        __LOG(debug, "onHeartbeatLost");
        if (l_cb)
        {
            l_cb(400);
        }
    }
    void onHeartbeatSuccess()
    {
        __LOG(debug, "onHeartbeatSuccess");
        if (s_cb)
        {
            s_cb();
        }
    }

    void start(ping_f fun)
    {

        // first we need to regard the heart beat is success
        // As ping is not send
        // At this time we should not call the ping function
        // as the connection may not set up yet
        // the worest case is to detect APP not avaliable net heart beat
        set_hb_success(true);
        __LOG(debug, "start heartbeat!");
        _timer->startForever(interval, [fun, this]() {
            if (this->get_hb_success())
            {
                onHeartbeatSuccess();
                _num_to_reconnect = config_center<void *>::instance()->get_properties_fields(get_genetic_gene(), dbw::PROP_HB_LOST_NUM, dbw::DEFAULT_HB_LOST_NUM);
            }
            else
            {
                _num_to_reconnect--;
                if (_num_to_reconnect < 1)
                {
                    _num_to_reconnect = 0;
                }
                //onHeartbeatLost();
            }

            {
                unsigned int tmp_num = _num_to_reconnect;
                __LOG(debug, "_num_to_reconnect is : " << tmp_num);
            }
            // set heartbeat status false, if hb success, it will set to true
            set_hb_success(false);
            int _tmp_num = _num_to_reconnect;
            if (_tmp_num < 1)
            {
                _num_to_reconnect = config_center<void *>::instance()->get_properties_fields(get_genetic_gene(), dbw::PROP_HB_LOST_NUM, dbw::DEFAULT_HB_LOST_NUM);
                onHeartbeatLost();
            }
            __LOG(debug, "call ping function : " << typeid(fun).name());
            if (fun)
            {
                fun();
            }
        });
    }
    bool stop()
    {
        _timer->stop();
        return true;
    }

    void set_interval(uint32_t iv) { interval = iv; }
    uint32_t get_interval() { return interval; }
    void set_hb_success_cb(hb_success_cb cb) { s_cb = cb; }
    void set_hb_lost_cb(hb_lost_cb cb) { l_cb = cb; }

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
    uint32_t interval;
    translib::Timer::ptr_p _timer;
    hb_success_cb s_cb;
    hb_lost_cb l_cb;

    std::atomic<bool> _success;
    std::atomic<unsigned int> _num_to_reconnect;
};
} // namespace heartBeat