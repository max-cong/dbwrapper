#pragma once
#include <typeinfo>
#include <unordered_map>
#include <mutex>
#include <memory>
namespace anySaver
{
template <typename keyType_t>
class anySaver
{
  public:
    anySaver() {}
    virtual ~anySaver() {}
    static std::shared_prt<anySaver<keyType_t>> instance()
    {
        static std::shared_prt<anySaver<keyType_t>> ins(new anySaver<keyType_t>);
        return ins;
    }
    // this is a glob, do not call this until all the client gone
    static void distroy(std::shared_ptr<anySaver<keyType_t>> &&ins)
    {
        if (ins)
        {
            // make sure no other data before delete
            if (ins->is_empty())
            {
                ins.reset();
            }
            else
            {
                __LOG(warn, "now try to delete anysaver of type :" << typeid(keyType_t).name() << ", there are data :");
                dumpData();
            }
        }
    }

    bool saveData(keyType_t key, std::string name, dbw::Any &&data)
    {
        std::lock_guard<std::mutex> lck(_mutex);
        // _anySaver[key][name]= data;
        if (_anySaver.find(key) != _anySaver.end())
        {
            _anySaver.at(key)[name] = data;
        }
        else
        {
            __LOG(debug, "no such key with type : " << typeid(key).name());
            std::map<std::string, dbw::Any> _data;
            _data[name] = data;
            _anySaver[key].swap(_data);
        }
        return true;
    }

    std::pair<dbw::Any, bool> getData(keyType_t key, std::string name)
    {
        std::lock_guard<std::mutex> lck(_mutex);
        auto it = _anySaver.find(key);
        if (it != _anySaver.end())
        {
            auto innerIt = (it->second).find(name);
            if (innerIt != (it->second).end())
            {
                return std::make_pair(_anySaver[key][name], true);
            }
        }
        return std::make_pair(nullptr, false);
    }

    void delData(keyType_t key, std::string name)
    {
        std::lock_guard<std::mutex> lck(_mutex);
        auto it = _anySaver.find(key);
        if (it != _anySaver.end())
        {
            auto innerIt = (it->second).find(name);
            if (innerIt != (it->second).end())
            {
                _anySaver[key].erase(name);
            }
        }
    }

    bool delKey(keyType_t key)
    {
        std::lock_guard<std::mutex> lck(_mutex);
        _anySaver.erase(key);
        return true;
    }

    bool isEmpty()
    {
        return _anySaver.empty();
    }

    void dumpData()
    {
        __LOG(debug, "data name in the any saver is : ");
        for (auto it : _anySaver)
        {
            __LOG(debug, "key type :" << typeid(it.first).name() << ", name is : ");
            for (auto innerIt : it.second)
            {
                __LOG(debug, "" << innerIt.first);
            }
        }
    }

  private:
    std::unordered_map<keyType_t, std::unordered_map<std::string, dbw::Any>> _anySaver;
    std::mutex _mutex;
};
} // namespace anySaver