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
#include "lbStrategy.hpp"
#include <random>
#include <mutex>
namespace lbStrategy
{
template <typename DIST_OBJ>
class discreteProbability : public lbStrategy<DIST_OBJ>
{
public:
    discreteProbability() : _gen(_rd())
    {
    }
    virtual bool init()
    {
        return true;
    }
    // note: there is no lock here
    // for performance, please check the return code during usage
    std::pair<DIST_OBJ, retStatus> get_obj() override
    {
        DIST_OBJ obj;
        // note: do this to supress build warning. this is redisAsyncContext*
        obj = NULL;
        if (this->_obj_vector.empty())
        {
            __LOG(warn, "there is no object to get!");
            return std::make_pair(obj, retStatus::NO_ENTRY);
        }
        try
        {
            obj = std::get<0>(this->_obj_vector.at(_dist(_gen)));
        }
        catch (const std::out_of_range &oor)
        {
            __LOG(error, "Out of Range error: " << oor.what());
            return std::make_pair(obj, retStatus::FAIL);
        }
        return std::make_pair(obj, retStatus::SUCCESS);
    }

    retStatus update() override
    {
        int vector_size = this->_obj_vector.size();
        if (!vector_size)
        {
            __LOG(debug, "this->_obj_vector is empty!");
            return retStatus::NO_ENTRY;
        }
        std::vector<double> init_list;
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
        return retStatus::SUCCESS;
    }

    std::random_device _rd;
    std::mt19937 _gen;
    std::discrete_distribution<int> _dist;
};
} // namespace lbStrategy