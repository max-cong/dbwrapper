/*
 * Copyright (c) 2016-20017 Max Cong <savagecm@qq.com>
 * this code can be found at https://github.com/maxcong001/logger
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

#include "logger/logger.hpp"
static const char black[] = {0x1b, '[', '1', ';', '3', '0', 'm', 0};
static const char red[] = {0x1b, '[', '1', ';', '3', '1', 'm', 0};
static const char yellow[] = {0x1b, '[', '1', ';', '3', '3', 'm', 0};
static const char blue[] = {0x1b, '[', '1', ';', '3', '4', 'm', 0};
static const char normal[] = {0x1b, '[', '0', ';', '3', '9', 'm', 0};

std::unique_ptr<loggerIface> _activeLogger(new logger(loggerIface::logLevel::error)); //nullptr;

logger::logger(loggerIface::logLevel level)
    : _logLevel(level)
{
    _id = 0;
    _buffer.fill(nullptr);
}
void logger::set_logLevel(loggerIface::logLevel level)
{
    _logLevel = level;
}
void logger::debug(const std::string &msg, const std::string &file, std::size_t line)
{

    if (_logLevel >= loggerIface::logLevel::debug)
    {

        std::cout << "[" << black << "DEBUG" << normal << "] [" << file << ":" << line << "] " << msg << std::endl;
    }
}

void logger::info(const std::string &msg, const std::string &file, std::size_t line)
{

    if (_logLevel >= loggerIface::logLevel::info)
    {

        std::cout << "[" << blue << "INFO " << normal << "] [" << file << ":" << line << "] " << msg << std::endl;
    }
}

void logger::warn(const std::string &msg, const std::string &file, std::size_t line)
{

    if (_logLevel >= loggerIface::logLevel::warn)
    {

        std::cout << "[" << yellow << "WARN " << normal << "] [" << file << ":" << line << "] " << msg << std::endl;
    }
}

void logger::error(const std::string &msg, const std::string &file, std::size_t line)
{

    if (_logLevel >= loggerIface::logLevel::error)
    {

        std::cerr << "[" << red << "ERROR" << normal << "] [" << file << ":" << line << "] " << msg << std::endl;
    }
}

void debug(const std::string &msg, const std::string &file, std::size_t line)
{
    if (_activeLogger)
        _activeLogger->debug(msg, file, line);
}

void info(const std::string &msg, const std::string &file, std::size_t line)
{
    if (_activeLogger)
        _activeLogger->info(msg, file, line);
}

void warn(const std::string &msg, const std::string &file, std::size_t line)
{
    if (_activeLogger)
        _activeLogger->warn(msg, file, line);
}

void error(const std::string &msg, const std::string &file, std::size_t line)
{
    if (_activeLogger)
        _activeLogger->error(msg, file, line);
}

void set_logLevel(loggerIface::logLevel level)
{
    if (_activeLogger)
        _activeLogger->set_logLevel(level);
}