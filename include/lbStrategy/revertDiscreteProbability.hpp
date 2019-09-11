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
    DBW_OPT<DIST_OBJ> getObj() override
    {

        if (this->getAvaliableObj().empty())
        {
            return DBW_NONE_OPT;
        }
        DIST_OBJ obj;
        try
        {
            obj = std::get<0>(this->getAvaliableObj().at(_dist(_gen)));
        }
        catch (const std::out_of_range &oor)
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "Out of Range error: " << oor.what());
            }
            return DBW_NONE_OPT;
        }
        return obj;
    }

    medis::retStatus update() override
    {
        std::vector<std::pair<DIST_OBJ, unsigned int>> avaliableObj = this->getAvaliableObj();
        int vector_size = avaliableObj.size();
        if (!vector_size)
        {
            if (CHECK_LOG_LEVEL(debug))
            {
                __LOG(debug, "this->getAvaliableObj() is empty!");
            }
            return medis::retStatus::NO_ENTRY;
        }
        std::vector<double> init_list;

        unsigned int max_weight = std::get<1>(*std::max_element(avaliableObj.begin(), avaliableObj.end(), [](const std::pair<DIST_OBJ, unsigned int> &a, const std::pair<DIST_OBJ, unsigned int> &b) -> bool {
            return (std::get<1>(a) > std::get<1>(b));
        }));
        unsigned int min_weight = std::get<1>(*std::min_element(avaliableObj.begin(), avaliableObj.end(), [](const std::pair<DIST_OBJ, unsigned int> &a, const std::pair<DIST_OBJ, unsigned int> &b) -> bool {
            return (std::get<1>(a) > std::get<1>(b));
        }));

        if (max_weight != min_weight)
        {
            for (int i = 0; i < vector_size; i++)
            {
                unsigned int tmp_weight = max_weight - std::get<1>(this->getAvaliableObj()[i]);
                init_list.push_back(tmp_weight);
            }
        }

        std::discrete_distribution<int> second_dist(init_list.begin(), init_list.end());
        _dist.param(second_dist.param());
        int _max_index = this->getAvaliableObj().size();
        return ((_max_index > 0) ? medis::retStatus::SUCCESS : medis::retStatus::NO_ENTRY);
    }

private:
    std::random_device _rd;
    std::mt19937 _gen;
    std::discrete_distribution<int> _dist;
};
} // namespace lbStrategy