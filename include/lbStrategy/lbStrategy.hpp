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
#include "logger/logger.hpp"
#include "util/defs.hpp"
#include <algorithm>
#include <utility>
#include <functional>

namespace lbStrategy
{
// note: this is not thread safe
template <typename LB_OBJ>
class lbStrategy
{
public:
    virtual ~lbStrategy() {}
    // Interface
    virtual DBW_OPT<LB_OBJ> getObj() = 0;
    virtual bool init() = 0;
    virtual medis::retStatus update() = 0;

    void setNoAvaliableCb(std::function<void()> const &cb)
    {
        _no_avaliable_cb = cb;
    }
    void setFirstAvaliableCb(std::function<void()> const &cb)
    {
        _first_avaliable_cb = cb;
    }

    DBW_OPT<LB_OBJ> getObjWithIndex(unsigned int index)
    {

        if (_obj_vector.empty())
        {
            __LOG(warn, "there is no entry here!");
            return DBW_NONE_OPT;
        }
        LB_OBJ obj;
        try
        {
            obj = std::get<0>(_obj_vector.at(index % (_obj_vector.size())));
        }
        catch (const std::out_of_range &oor)
        {
            __LOG(error, "Out of Range error: " << oor.what());
            return DBW_NONE_OPT;
        }
        return obj;
    }

    // common function
    virtual medis::retStatus addObj(LB_OBJ obj, unsigned int weight = 0)
    {
        __LOG(debug, "now add one object!");
        return updateObj(obj, weight);
    }

    medis::retStatus delObj(const LB_OBJ obj)
    {
        unsigned int _avaliable_obj_before = getAvaliableObj().size();

        for (auto it = _obj_vector.begin(); it != _obj_vector.end();)

        {
            if (std::get<0>(*it) == obj)
            {
                __LOG(warn, "now delete one object from _obj_vector");
                it = _obj_vector.erase(it);
            }
            else
            {
                it++;
            }
        }
        for (auto it = _inactive_obj_vector.begin(); it != _inactive_obj_vector.end();)

        {
            if ((*it) == obj)
            {
                __LOG(warn, "now delete one object from _inactive_obj_vector");
                it = _inactive_obj_vector.erase(it);
            }
            else
            {
                it++;
            }
        }

        __LOG(warn, " size of _obj_vector is : " << _obj_vector.size() << ", size of _inactive_obj_vector is : " << _inactive_obj_vector.size());
        unsigned int _avaliable_obj_after = getAvaliableObj().size();
        // note: you need to make sure there is connection before and then there is no connection, we call no avaliable callback
        // that is : if you do not have connection form the first time. no avaliable callback will not triger
        if (_avaliable_obj_before && !_avaliable_obj_after && _no_avaliable_cb)
        {
            _no_avaliable_cb();
        }

        return update();
    }

    unsigned int getWeight(LB_OBJ obj)
    {

        for (auto it : _obj_vector)
        {
            if (std::get<0>(it) == obj)
            {
                return std::get<1>(it);
            }
        }
        return 0;
    }

    medis::retStatus updateObj(LB_OBJ obj, unsigned int weight = 0)
    {
        __LOG(debug, " update obj is called, weight is : " << weight);
        unsigned int _avaliable_obj_before = getAvaliableObj().size();

        if (!weight)
        {
            updateObjZero(obj);
        }
        else // weight is not 0
        {
            updateObjNotZero(obj, weight);
        }

        unsigned int _avaliable_obj_after = getAvaliableObj().size();
        if (!_avaliable_obj_after && _avaliable_obj_before && _no_avaliable_cb)
        {
            _no_avaliable_cb();
        }
        if (_avaliable_obj_before == 0 && _avaliable_obj_after > 0)
        {
            __LOG(debug, "first obj");
            if (_first_avaliable_cb)
            {
                _first_avaliable_cb();
            }
        }
        return update();
    }

    virtual medis::retStatus incWeight(LB_OBJ obj, unsigned int weight)
    {
        unsigned int _weight = 0;
        _weight = getWeight(obj);
        _weight += weight;
        return updateObj(obj, _weight);
    }
    virtual medis::retStatus decWeight(LB_OBJ obj, unsigned int weight)
    {
        unsigned int _weight = 0;
        _weight = getWeight(obj);
        _weight -= weight;
        if (_weight < 0)
        {
            _weight = 0;
        }
        return updateObj(obj, _weight);
    }

    // get all the object, include active and inactive
    virtual std::vector<std::pair<LB_OBJ, unsigned int>> getAllObj()
    {
        std::vector<std::pair<LB_OBJ, unsigned int>> ret;
        ret.insert(ret.begin(), _obj_vector.begin(), _obj_vector.end());
        for (auto it : _inactive_obj_vector)
        {
            ret.push_back(std::make_pair(it, 0));
        }
        return ret;
    }
    virtual bool clearInfo()
    {
        __LOG(debug, "[clear info]--------------")
        _obj_vector.clear();
        _inactive_obj_vector.clear();
        update();
        return true;
    }

    std::vector<std::pair<LB_OBJ, unsigned int>> getAvaliableObj()
    {
        return _obj_vector;
    }
    std::vector<std::pair<LB_OBJ, unsigned int>> _obj_vector;
    std::vector<LB_OBJ> _inactive_obj_vector;
    std::recursive_mutex _mutex;

    std::function<void()> _no_avaliable_cb;
    std::function<void()> _first_avaliable_cb;

private:
    void updateObjZero(LB_OBJ obj)
    {
        // if the obj is in active list, remove it
        for (auto it = _obj_vector.begin(); it != _obj_vector.end();)
        {
            __LOG(debug, "loop _obj_vector");
            if (std::get<0>(*it) == obj)
            {
                __LOG(debug, "found obj, erase it");
                it = _obj_vector.erase(it);
            }
            else
            {
                it++;
            }
        }

        bool found = false;
        __LOG(debug, "loop inactive vector, there are [" << _inactive_obj_vector.size() << "] obj in the inactive vector");
        for (auto it = _inactive_obj_vector.begin(); it != _inactive_obj_vector.end();)
        {
            if ((*it) == obj)
            {
                __LOG(warn, "obj is in the inactive obj");
                found = true;
                it++;
            }
            else
            {
                it++;
            }
        }
        if (!found)
        {
            _inactive_obj_vector.push_back(obj);
        }
    }
    void updateObjNotZero(LB_OBJ obj, unsigned int weight)
    {
        for (auto it = _inactive_obj_vector.begin(); it != _inactive_obj_vector.end();)
        {
            __LOG(debug, "loop inactive thread");
            if ((*it) == obj)
            {
                it = _inactive_obj_vector.erase(it);
            }
            else
            {
                it++;
            }
        }

        for (auto it = _obj_vector.begin(); it != _obj_vector.end();)
        {
            __LOG(debug, "loop _obj_vector");
            if (std::get<0>(*it) == obj)
            {
                __LOG(debug, "found obj, erase it");
                it = _obj_vector.erase(it);
            }
            else
            {
                it++;
            }
        }
        _obj_vector.push_back(std::make_pair(obj, weight));
    }
};
} // namespace lbStrategy