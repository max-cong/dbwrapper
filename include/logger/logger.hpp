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

#include <memory>
#include <mutex>
#include <string>   // std::string
#include <iostream> // std::cout
#include <sstream>  // std::ostringstream
#include <array>
#include <atomic>
#include <memory>

typedef std::basic_ostream<char> tostream;
typedef std::basic_istream<char> tistream;
typedef std::basic_ostringstream<char> tostringstream;
typedef std::basic_istringstream<char> tistringstream;
static const char black[] = {0x1b, '[', '1', ';', '3', '0', 'm', 0};
static const char red[] = {0x1b, '[', '1', ';', '3', '1', 'm', 0};
static const char yellow[] = {0x1b, '[', '1', ';', '3', '3', 'm', 0};
static const char blue[] = {0x1b, '[', '1', ';', '3', '4', 'm', 0};
static const char normal[] = {0x1b, '[', '0', ';', '3', '9', 'm', 0};
class loggerIface
{
public:
	//! log level
	enum class logLevel
	{
		error = 0,
		warn = 1,
		info = 2,
		debug = 3
	};

	loggerIface() = default;
	virtual ~loggerIface() = default;

	loggerIface(const loggerIface &) = default;
	loggerIface &operator=(const loggerIface &) = default;

	virtual void set_logLevel(loggerIface::logLevel level) = 0;
	virtual void debug(const std::string &msg, const std::string &file, std::size_t line) = 0;
	virtual void info(const std::string &msg, const std::string &file, std::size_t line) = 0;
	virtual void warn(const std::string &msg, const std::string &file, std::size_t line) = 0;
	virtual void error(const std::string &msg, const std::string &file, std::size_t line) = 0;
};

class logger : public loggerIface
{
public:
	explicit logger(loggerIface::logLevel level = loggerIface::logLevel::info) : _logLevel(level)
	{
	}
	~logger() = default;

	logger(const logger &) = default;
	logger &operator=(const logger &) = default;

	void set_logLevel(loggerIface::logLevel level)
	{
		_logLevel = level;
	}
	void debug(const std::string &msg, const std::string &file, std::size_t line)
	{

		if (_logLevel >= loggerIface::logLevel::debug)
		{

			std::cout << "[" << black << "DEBUG" << normal << "] [" << file << ":" << line << "] " << msg << std::endl;
		}
	}
	void info(const std::string &msg, const std::string &file, std::size_t line)
	{

		if (_logLevel >= loggerIface::logLevel::info)
		{

			std::cout << "[" << blue << "INFO " << normal << "] [" << file << ":" << line << "] " << msg << std::endl;
		}
	}

	void warn(const std::string &msg, const std::string &file, std::size_t line)
	{

		if (_logLevel >= loggerIface::logLevel::warn)
		{

			std::cout << "[" << yellow << "WARN " << normal << "] [" << file << ":" << line << "] " << msg << std::endl;
		}
	}

	void error(const std::string &msg, const std::string &file, std::size_t line)
	{

		if (_logLevel >= loggerIface::logLevel::error)
		{

			std::cerr << "[" << red << "ERROR" << normal << "] [" << file << ":" << line << "] " << msg << std::endl;
		}
	}

private:
	loggerIface::logLevel _logLevel;
};

void debug(const std::string &msg, const std::string &file, std::size_t line);
void info(const std::string &msg, const std::string &file, std::size_t line);
void warn(const std::string &msg, const std::string &file, std::size_t line);
void error(const std::string &msg, const std::string &file, std::size_t line);
void set_logLevel(loggerIface::logLevel level);

//#define __LOGGING_ENABLED

#ifdef __LOGGING_ENABLED
#define __LOG(level, msg)                              \
                                                       \
	{                                                  \
		tostringstream var;                            \
		var << "[fuction:" << __func__ << "] " << msg; \
		level(var.str(), __FILE__, __LINE__);          \
	}
#else
#define __LOG(level, msg)
#endif /* __LOGGING_ENABLED */