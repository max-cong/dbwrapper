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
    std::pair<LB_OBJ, retStatus> get_obj() override
    {
        LB_OBJ obj;
        // note: do this to supress build warning. this is redisAsyncContext*
        obj = NULL;
        if (this->_obj_vector.empty() || !_max_index)
        {
            return std::make_pair(obj, retStatus::NO_ENTRY);
        }
        try
        {
            _index++;
            return std::make_pair(std::get<0>(this->_obj_vector.at(_index % (this->_max_index))), retStatus::SUCCESS);
        }
        catch (const std::out_of_range &oor)
        {
            __LOG(error, "Out of Range error: " << oor.what());

            return std::make_pair(obj, retStatus::FAIL);
        }
        return std::make_pair(obj, retStatus::SUCCESS);
    }

    // for round robbin, if the weight is 0, that mean we should delete the obj the default weight_ is 10
    
    virtual retStatus add_obj(LB_OBJ obj, unsigned int weight = 10)
    {
        return this->update_obj(obj, weight);
    }

    virtual bool init()
    {
        return true;
    }

    retStatus update() override
    {
        _max_index = this->_obj_vector.size();
        return retStatus::SUCCESS;
    }

private:
    unsigned int _index;
    unsigned int _max_index;
};
} // namespace lbStrategy