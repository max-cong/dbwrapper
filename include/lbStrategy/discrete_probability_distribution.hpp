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
#include "load_balance_strategy.hpp"
#include <random>
#include <mutex>

template <typename DIST_OBJ>
class DPD : public LB_strategy<DIST_OBJ>
{
  public:
    DPD() : _gen(_rd())
    {
    }
    virtual bool init()
    {
        return true;
    }
    // note: there is no lock here
    // for performance, please check the return code during usage
    std::pair<DIST_OBJ, ret_status> get_obj(int index = 0) override
    {
        DIST_OBJ obj;
        if (this->_obj_vector.empty())
        {
            __LOG(warn, "there is no object to get!");
            return std::make_pair(obj, ret_status::NO_ENTRY);
        }
        try
        {
            // should 0 to sizeof _obj_vector
            int _rand_num = _dist(_gen);
            if (_rand_num == 0)
            {
                // if the radom number is 0, that means there is no avaliable entry
                return std::make_pair(obj, ret_status::NO_ENTRY);
            }
            if (index == 0)
            {
                // if _rand_num is not 0, it will start from 1
                obj = std::get<0>(this->_obj_vector.at(_rand_num - 1));
            }
            else
            {
                std::lock_guard<std::recursive_mutex> lck(this->_mutex);
                // add lock here, as we need to count the sizeof vector
                obj = std::get<0>(this->_obj_vector.at(index % ((this->_obj_vector).size())));
            }
        }
        catch (const std::out_of_range &oor)
        {
            __LOG(error, "Out of Range error: " << oor.what());
            return std::make_pair(obj, ret_status::FAIL);
        }
        return std::make_pair(obj, ret_status::SUCCESS);
    }

    ret_status update() override
    {
        int vector_size = 0;

        std::lock_guard<std::recursive_mutex> lck(this->_mutex);

        vector_size = this->_obj_vector.size();
        if (!vector_size)
        {
            __LOG(debug, "this->_obj_vector is empty!");
            // to do : need to return here?
        }
        std::vector<double> init_list;
        // as if all the weight is 0, it will always return 0, add 0 to stub it,
        // then we will get from 1, if we get 0, then there is no entry
        init_list.push_back(0);
        //std::array<double, 10000> init_list;
        __LOG(debug, "weight is :");
        for (int i = 0; i < vector_size; i++)
        {
            init_list.push_back(std::get<1>(this->_obj_vector[i]));
            __LOG(debug, "--> " << std::get<1>(this->_obj_vector[i]));
        }
        std::discrete_distribution<int> second_dist(init_list.begin(), init_list.end());
        auto _param = second_dist.param();
        _dist.param(_param);
        _dist.reset();
        if (!vector_size)
        {
            return ret_status::NO_ENTRY;
        }
        else
        {
            return ret_status::SUCCESS;
        }
    } 

    std::random_device _rd;
    std::mt19937 _gen;
    std::discrete_distribution<int> _dist;
};
