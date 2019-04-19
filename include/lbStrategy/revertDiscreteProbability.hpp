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
#include <utility> // for pair
#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath>
namespace lbStrategy
{
template <typename DIST_OBJ>
class revertDiscreteProbability : public lbStrategy<DIST_OBJ>
{
public:
    revertDiscreteProbability() : _gen(_rd())
    {
    }
    virtual bool init()
    {
        return true;
    }
    // note: there is no lock here
    std::pair<DIST_OBJ, retStatus> get_obj(int index = 0) override
    {

        DIST_OBJ obj;
        if (this->_obj_vector.empty())
        {
            return std::make_pair(obj, retStatus::NO_ENTRY);
        }
        try
        {
            if (index == 0)
            {
                obj = std::get<0>(this->_obj_vector.at(_dist(_gen)));
            }
            else
            {
                obj = std::get<0>(this->_obj_vector.at(index % ((this->_obj_vector).size())));
            }
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
            return std::make_pair(obj, retStatus::NO_ENTRY);
        }
        std::vector<double> init_list;
        unsigned int max_weight = std::get<1>(*std::max_element(_obj_vector.begin(), _obj_vector.end()));
        unsigned int min_weight = std::get<1>(*std::min_element(_obj_vector.begin(), _obj_vector.end()));

        if (max_weight != min_weight)
        {
            for (int i = 0; i < vector_size; i++)
            {
                unsigned int tmp_weight = max_weight - std::get<1>(this->_obj_vector[i]);
                init_list.push_back(tmp_weight);
            }
        }

        std::discrete_distribution<int> second_dist(init_list.begin(), init_list.end());
        _dist.param(second_dist.param());

        return retStatus::SUCCESS;
    }

    std::random_device _rd;
    std::mt19937 _gen;
    std::discrete_distribution<int> _dist;
};
} // namespace lbStrategy