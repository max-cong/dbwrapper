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
#include <list>

enum class REDIS_COMMAND_TYPE : std::uint32_t
{
    TASK_REDIS_PUT,
    TASK_REDIS_GET,
    TASK_REDIS_DEL,
    TASK_REDIS_ADD_CONN,
    TASK_REDIS_DEL_CONN,
    TASK_REDIS_PING,
    TASK_REDIS_MAX

};
namespace buildRedisCommand
{

// interface form APP API to redis RSP message
template <typename COMMAND_KEY = std::string, typename COMMAND_VALUE = std::string, typename COMMAND_ARGS = std::nullptr_t> // to do typename... COMMAND_ARGS>
class buildRedisCommand
{
    using List = std::list<std::string>;

public:
#if __cplusplus >= 201703L
    using key_type = std::remove_const_t<std::remove_reference_t<COMMAND_KEY>>;
    using value_type = std::remove_const_t<std::remove_reference_t<COMMAND_VALUE>>;
#endif
    buildRedisCommand() = default;

    static std::string get_format_command(REDIS_COMMAND_TYPE type, COMMAND_KEY key, COMMAND_VALUE value, COMMAND_ARGS args = nullptr) // to do COMMAND_ARGS... args)
    {
        switch (type)
        {
        case REDIS_COMMAND_TYPE::TASK_REDIS_PUT:
#if __cplusplus >= 201703L
            if constexpr (detail::is_string_v<COMMAND_KEY>)
            {
                if constexpr (detail::is_string_v<COMMAND_VALUE>)
                {
                    __LOG(error, "Put command");
                    List _list;
                    _list.emplace_back("SET");
                    _list.emplace_back(key);
                    _list.emplace_back(value);
                    return redis_formatCommand(_list);
                }
                else
                {
                }
            }
            else
            {
            }
#else
            if (std::is_same<COMMAND_KEY, std::string>::value && std::is_same<COMMAND_VALUE, std::string>::value)
            {

                __LOG(debug, "Put command, key is : " << key << ", value is : " << value);
                List _list;
                _list.emplace_back("SET");
                _list.emplace_back(key);
                _list.emplace_back(value);
                return redis_formatCommand(_list);
            }
#endif
            break;
        case REDIS_COMMAND_TYPE::TASK_REDIS_GET:
            break;
        case REDIS_COMMAND_TYPE::TASK_REDIS_DEL:
            break;
        case REDIS_COMMAND_TYPE::TASK_REDIS_ADD_CONN:
            break;
        case REDIS_COMMAND_TYPE::TASK_REDIS_DEL_CONN:
            break;
        case REDIS_COMMAND_TYPE::TASK_REDIS_PING:
            break;
        default:
            __LOG(warn, "unsupport message type!");
            break;
        }
        return "";
    }

    static std::string redis_formatCommand(List &argv)
    {
        __LOG(debug, "[redis_formatCommand]");
        std::ostringstream buffer;
        buffer << "*" << argv.size() << "\r\n";
        List::const_iterator iter = argv.begin();
        while (iter != argv.end())
        {
            buffer << "$" << iter->size() << "\r\n";
            buffer << *iter++ << "\r\n";
        }
        return buffer.str();
    }
};
} // namespace buildRedisCommand