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

namespace lbStrategy
{
template <typename LB_OBJ>
class roundRobbin : public lbStrategy<LB_OBJ>
{
public:
    roundRobbin() : _index(0)
    {
        _max_index = 0;
    }
    ~roundRobbin() {}
    // note: there is no lock here
    DBW_OPT<LB_OBJ> getObj() override
    {
        if (this->getAvaliableObj().empty() || !_max_index)
        {

            return DBW_NONE_OPT;
        }

        _index++;
        return std::get<0>((this->getAvaliableObj())[_index % (this->_max_index)]);
    }

    // for round robbin, if the weight is 0, that mean we should delete the obj the default weight_ is 10

    virtual medis::retStatus addObj(LB_OBJ obj, unsigned int weight = 1)
    {
        return this->updateObj(obj, weight);
    }

    virtual bool init()
    {
        return true;
    }

    medis::retStatus update() override
    {
        _max_index = this->getAvaliableObj().size();
        return ((_max_index > 0) ? medis::retStatus::SUCCESS : medis::retStatus::NO_ENTRY);
    }

private:
    unsigned int _index;
    unsigned int _max_index;
};
} // namespace lbStrategy