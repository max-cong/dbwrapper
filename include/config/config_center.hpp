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
#include <map>
#include <boost/lexical_cast.hpp>
typedef std::map<std::string, std::string> cfg_prop_map;
namespace config
{
template <typename config_key_type_t>
class config_center
{
  public:
	config_center() {}
	virtual ~config_center() {}
	static config_center<config_key_type_t> *instance()
	{
		static config_center<config_key_type_t> *ins = new config_center<config_key_type_t>();
		return ins;
	}
	// this is a glob, do not call this until all the client gone
	static void distroy(config_center<config_key_type_t> *ins)
	{
		if (!ins)
		{
			delete ins;
			ins = NULL;
		}
	}

	cfg_prop_map get_properties(config_key_type_t key)
	{
		std::lock_guard<std::mutex> lck(_mutex);
		cfg_prop_map prop;
		try
		{
			prop = _properties_map.at(key);
		}
		catch (const std::out_of_range &oor)
		{
			__LOG(error, "get properties fail, no such a key");
		}
		return prop;
	}

	template <typename T>
	T get_properties_fields(config_key_type_t key, std::string field, const T default_value)
	{
		std::lock_guard<std::mutex> lck(_mutex);
		cfg_prop_map prop;
		std::string _field;
		try
		{
			prop = _properties_map.at(key);
		}
		catch (const std::out_of_range &oor)
		{
			// if build error, just comment this log
			__LOG(debug, "get properties fail, no such a key : " << (void *)key << ". Use default value : " << default_value << ", field is : " << field);
			_field.clear();
			return default_value;
		}
		try
		{
			_field = prop.at(field);
		}
		catch (const std::out_of_range &oor)
		{
			__LOG(debug, "get properties fail, no such a field : " << field << ". use default value : " << default_value);
			_field.clear();
			return default_value;
		}

		try
		{
			return _field.empty() ? default_value : boost::lexical_cast<T>(_field);
		}
		catch (boost::bad_lexical_cast &e)
		{
			return default_value;
		}
	}
	bool update_properties(config_key_type_t key, std::string field, std::string value)
	{

		std::lock_guard<std::mutex> lck(_mutex);
		// get prop

		cfg_prop_map prop;
		try
		{
			prop = _properties_map.at(key);
		}
		catch (const std::out_of_range &oor)
		{
			__LOG(error, "get properties fail, no such a key");
			return false;
		}
		// update prop
		std::string _value;
		try
		{
			_value = prop.at(field);
		}
		catch (const std::out_of_range &oor)
		{
			__LOG(error, "get prop value fail, no such a field");
			return false;
		}
		if (!_value.compare(value))
		{
			__LOG(debug, "value is the same, no need to update!");
		}
		else
		{
			prop[field] = value;
		}
		return true;
	}
	bool update_properties(config_key_type_t key, cfg_prop_map props)
	{
		{
			__LOG(debug, "update config, key is : " << (void *)key << ", data is:");
			for (auto it : props)
			{
				__LOG(debug, "field is : " << it.first << ", value is : " << it.second);
			}
		}
		auto tmp_prop = get_properties(key);
		{
			std::lock_guard<std::mutex> lck(_mutex);
			for (auto it : props)
			{
				tmp_prop[it.first] = it.second;
			}
			//tmp_prop.insert(props.begin(), props.end());
		}
		set_properties(key, tmp_prop);
		return true;
	}
	bool set_properties(config_key_type_t key, cfg_prop_map prop)
	{
		std::lock_guard<std::mutex> lck(_mutex);
		_properties_map[key] = prop;
		return true;
	}
	bool del_properties(config_key_type_t key)
	{
		std::lock_guard<std::mutex> lck(_mutex);
		_properties_map.erase(key);
		return true;
	}

	std::map<config_key_type_t, cfg_prop_map> _properties_map;
	std::mutex _mutex;
};
}