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
enum class REDIS_COMMAND_TYPE : std::uint32_t
{
    TASK_REDIS_PUT,
    TASK_REDIS_MPUT,
    TASK_REDIS_GET,
    TASK_REDIS_MGET,
    TASK_REDIS_DEL,
    TASK_REDIS_MDEL,
    TASK_REDIS_PUB,
    TASK_REDIS_SUB,
    TASK_REDIS_UNSUB,
    TASK_REDIS_PING,
    TASK_REDIS_ADD_CONN,
    TASK_REDIS_DEL_CONN,

    TASK_REDIS_MAX
};

class redis_formatCommand
{
public:
    static std::string toString(std::list<std::string> const &argv)
    {
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "[redis_formatCommand]");
        }
        if (argv.empty())
        {
            if (CHECK_LOG_LEVEL(warn))
            {
                __LOG(warn, "command list is empty");
            }
            return "";
        }
        std::ostringstream buffer;
        buffer << "*" << argv.size() << "\r\n";
        std::list<std::string>::const_iterator iter = argv.begin();
        while (iter != argv.end())
        {
            buffer << "$" << iter->size() << "\r\n";
            buffer << *iter++ << "\r\n";
        }
        return buffer.str();
    }
};