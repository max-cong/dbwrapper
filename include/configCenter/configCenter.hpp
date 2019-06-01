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
#include <typeinfo>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <boost/lexical_cast.hpp>
namespace configCenter
{
// note: the shared_instance of this class will live until system gone
using cfgPropMap = std::unordered_map<std::string, std::string>;
template <typename cfgKeyType_t = void *>
class configCenter
{
public:
	configCenter() {}
	virtual ~configCenter() {}
	static std::shared_ptr<configCenter<cfgKeyType_t>> instance()
	{
		static std::shared_ptr<configCenter<cfgKeyType_t>> ins(new configCenter<cfgKeyType_t>);
		return ins;
	}
	// this is a glob, do not call this until all the client gone
	static void distroy(std::shared_ptr<configCenter<cfgKeyType_t>> &&ins)
	{
		if (!ins)
		{
			ins.reset();
		}
	}

	cfgPropMap getProperties(cfgKeyType_t key)
	{
		std::lock_guard<std::mutex> lck(_mutex);
		if (_propertiesMap.find(key) != _propertiesMap.end())
		{
			return _propertiesMap[key];
		}
		else
		{
			cfgPropMap prop;
			return prop;
		}
	}

	template <typename T>
	T getPropertiesField(cfgKeyType_t key, std::string field, const T default_value)
	{
		std::lock_guard<std::mutex> lck(_mutex);

		auto it = _propertiesMap.find(key);
		if (it != _propertiesMap.end())
		{
			auto innerIt = _propertiesMap[key].find(field);
			if (innerIt != _propertiesMap[key].end())
			{
				auto ret = _propertiesMap[key][field];
				try
				{
					return ret.empty() ? default_value : boost::lexical_cast<T>(ret);
				}
				catch (boost::bad_lexical_cast &e)
				{
					__LOG(warn, "error finding field : " << field << ", type case fail. dest type is : " << typeid(T).name());
				}
			}
		}
		return default_value;
	}

	bool updateProperties(cfgKeyType_t key, std::string field, std::string value)
	{

		std::lock_guard<std::mutex> lck(_mutex);

		auto it = _propertiesMap.find(key);
		if (it != _propertiesMap.end())
		{
			auto innerIt = _propertiesMap[key].find(field);
			if (innerIt != _propertiesMap[key].end())
			{
				_propertiesMap[key][field] = value;
				return true;
			}
			else
			{
				if (CHECK_LOG_LEVEL(debug))
				{
					__LOG(debug, "did not find field with name : " << field);
				}
				return false;
			}
		}
		else
		{
			if (CHECK_LOG_LEVEL(debug))
			{
				__LOG(debug, "did not find key with type : " << typeid(key).name());
			}
			return false;
		}
		return false;
	}
	bool updateProperties(cfgKeyType_t key, cfgPropMap const &props)
	{
		std::lock_guard<std::mutex> lck(_mutex);
		auto it = _propertiesMap.find(key);
		if (it != _propertiesMap.end())
		{
			for (auto innerIt : props)
			{
				_propertiesMap[key][it->first] = it->second;
			}
			return true;
		}
		else
		{
			if (CHECK_LOG_LEVEL(debug))
			{
				__LOG(debug, "did not find key with type : " << typeid(key).name());
			}
			return false;
		}
	}
	bool setProperties(cfgKeyType_t key, cfgPropMap const &prop)
	{
		std::lock_guard<std::mutex> lck(_mutex);
		_propertiesMap[key] = prop;
		return true;
	}
	bool delProperties(cfgKeyType_t key)
	{
		std::lock_guard<std::mutex> lck(_mutex);
		_propertiesMap.erase(key);
		return true;
	}

	std::map<cfgKeyType_t, cfgPropMap> _propertiesMap;
	std::mutex _mutex;
};
} // namespace configCenter