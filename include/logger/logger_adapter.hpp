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
#include <memory>
#include <mutex>
#include <string>   // std::string
#include <iostream> // std::cout
#include <sstream>  // std::ostringstream
#include "logger/logger.hpp"
#define DBW_LOG __FILE__, __LINE__, __func__
#define logger SimpleLogger::instance()
#define LOG_BUFFER_SIZE 10240
class Logger
{
  public:
    enum Level
    {
        ALL,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        OFF
    };
    Logger() : level(ERROR) {}
    Logger(Level lev) : level(lev) {}
    virtual ~Logger() {}
    static Logger *instance()
    {
        static Logger *ins = new Logger();
        return ins;
    }

    void setLevel(Level lev) { this->level = lev; }
    Level getLevel() { return this->level; }

    static inline std::string logLevelString(const Level lev)
    {
        return (lev < ALL || lev > OFF) ? "UNDEFINED_LOGLEVEL" : (const std::string[]){"ALL", "DBUG", "INFO", "WARN", "ERROR", "FATAL", "OFF"}[lev];
    }

    virtual void debug(const char *file, int line, const char *func, const char *fmt, ...)
    {
        std::string tmp_str;
        tmp_str.append(file).append(" ").append(std::to_string(line)).append(func).append(" ").append(fmt);
        std::cout << tmp_str << std::endl;
    }
    virtual void info(const char *file, int line, const char *func, const char *fmt, ...)
    {
        std::string tmp_str;
        tmp_str.append(file).append(" ").append(std::to_string(line)).append(func).append(" ").append(fmt);
        std::cout << tmp_str << std::endl;
    }
    virtual void warn(const char *file, int line, const char *func, const char *fmt, ...)
    {
        std::string tmp_str;
        tmp_str.append(file).append(" ").append(std::to_string(line)).append(func).append(" ").append(fmt);
        std::cout << tmp_str << std::endl;
    }
    virtual void error(const char *file, int line, const char *func, const char *fmt, ...)
    {
        std::string tmp_str;
        tmp_str.append(file).append(" ").append(std::to_string(line)).append(func).append(" ").append(fmt);
        std::cout << tmp_str << std::endl;
    }
    virtual void fatal(const char *file, int line, const char *func, const char *fmt, ...)
    {
        std::string tmp_str;
        tmp_str.append(file).append(" ").append(std::to_string(line)).append(func).append(" ").append(fmt);
        std::cout << tmp_str << std::endl;
    }
    Level level;
};

class SimpleLogger : virtual public Logger
{
  public:
    SimpleLogger() : Logger(), buf_size(LOG_BUFFER_SIZE) {}
    SimpleLogger(Level lev) : Logger(lev), buf_size(LOG_BUFFER_SIZE) {}

    static SimpleLogger *instance()
    {
        static SimpleLogger *ins = new SimpleLogger(ERROR);
        return ins;
    }

    virtual void debug(const char *file, int line, const char *func, const char *fmt, ...);
    virtual void info(const char *file, int line, const char *func, const char *fmt, ...);
    virtual void warn(const char *file, int line, const char *func, const char *fmt, ...);
    virtual void error(const char *file, int line, const char *func, const char *fmt, ...);
    virtual void fatal(const char *file, int line, const char *func, const char *fmt, ...);

    virtual ~SimpleLogger() {}

  private:
    const size_t buf_size;
    size_t getBufSize() { return this->buf_size; }
    void simpleLogHandler(const char *file, int line, const char *func, Level lev, const char *fmt, va_list args);
};