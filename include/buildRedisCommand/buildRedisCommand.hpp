#pragma once

#include <list>
enum class REDIS_MSG_TYPE : std::uint32_t
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

    static std::string get_format_command(REDIS_MSG_TYPE type, COMMAND_KEY key, COMMAND_VALUE value, COMMAND_ARGS args = nullptr) // to do COMMAND_ARGS... args)
    {
        switch (type)
        {
        case REDIS_MSG_TYPE::TASK_REDIS_PUT:
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
        case REDIS_MSG_TYPE::TASK_REDIS_GET:
            break;
        case REDIS_MSG_TYPE::TASK_REDIS_DEL:
            break;
        case REDIS_MSG_TYPE::TASK_REDIS_ADD_CONN:
            break;
        case REDIS_MSG_TYPE::TASK_REDIS_DEL_CONN:
            break;
        case REDIS_MSG_TYPE::TASK_REDIS_PING:
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