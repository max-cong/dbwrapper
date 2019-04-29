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
#include <type_traits>
#include <stdexcept>

#include "redisCommandUtil.hpp"
#include "util/nonCopyable.hpp"
#if __cplusplus >= 201703L
#define medisConstExpr constexpr
#else
#define medisConstExpr
#endif
namespace buildRedisCommand
{
template <typename KEY_TYPE, typename VALUE_TYPE>
class redisSet : public nonCopyable
{
public:
    redisSet() = delete;
    redisSet(KEY_TYPE key, VALUE_TYPE value)
    {
    }
    std::string toString() { return ""; }
};

template <>
class redisSet<std::string, std::string> : public nonCopyable
{
public:
    redisSet() = delete;
    redisSet(std::string key, std::string value)
    {
        // may use ostream for performance
        // to do
        __LOG(debug, "[redisSet] key is : " << key << ", value is : " << value);
        _list.emplace_back("SET");
        _list.emplace_back(key);
        _list.emplace_back(value);
    }
    std::string toString()
    {
        return redis_formatCommand::toString(_list);
    }
    std::list<std::string> _list;
};
template <typename KEY_TYPE, typename VALUE_TYPE>
class redisMSet : public nonCopyable
{
public:
    redisMSet() = delete;
    redisMSet(KEY_TYPE key, VALUE_TYPE value)
    {
    }
    std::string toString() { return ""; }
};
template <>
class redisMSet<std::list<std::string>, std::list<std::string>> : public nonCopyable
{
public:
    redisMSet() = delete;
    redisMSet(std::list<std::string> key, std::list<std::string> value)
    {
        if (key.size() == value.size())
        {
            _list.emplace_back("MSET");
            for (auto it : key)
            {
                _list.push_back(key.front());
                key.pop_front();
                _list.push_back(value.front());
                value.pop_front();
            }
        }
    }
    std::string toString()
    {
        return redis_formatCommand::toString(_list);
    }
    std::list<std::string> _list;
};

// interface form APP API to redis RSP message
template <typename COMMAND_KEY, typename COMMAND_VALUE>
class buildRedisCommand
{

public:
    template <typename TYPE>
    static constexpr bool keyCheck()
    {
        return (std::is_same<typename std::decay<TYPE>::type, COMMAND_KEY>::value);
    }
    template <typename TYPE>
    static constexpr bool valueCheck()
    {
        return (std::is_same<typename std::decay<TYPE>::type, COMMAND_VALUE>::value);
    }

    buildRedisCommand() = default;

    static std::string get_format_command(COMMAND_KEY key, COMMAND_VALUE value)
    {
        if
            medisConstExpr(keyCheck<std::string>() && valueCheck<std::string>())
            {
                __LOG(debug, "Put command, key and value both string. key is : " << key << ", value is : " << value);
                return redisSet<COMMAND_KEY, COMMAND_VALUE>(key, value).toString();
            }
        else if
            medisConstExpr(keyCheck<std::list<std::string>>() && valueCheck<std::list<std::string>>())
            {
                return redisMSet<COMMAND_KEY, COMMAND_VALUE>(key, value).toString();
            }
        else
        {
            __LOG(debug, "Put command, key or value type is not matched, key type is : " << typeid(key).name() << ". value type is : " << typeid(value).name());
        }

        return "";
    }
};

} // namespace buildRedisCommand
