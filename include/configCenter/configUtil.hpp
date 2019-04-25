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

#include "configCenter.hpp"
#include <string>

// Property names
const std::string PROP_HOST = "PROP_HOST"; // the remote host IP or FQDN
const std::string DEFAULT_HOST = "127.0.0.1";

const std::string PROP_PORT = "PROP_PORT"; // the remote host IP or FQDN
const std::string DEFAULT_PORT = "6379";

const std::string DEFAULT_HB_LOST_NUM = "5";
const std::string PROP_HB_LOST_NUM = "PROP_HB_LOST_NUM";

const std::string DEFAULT_LOAD_BALANCE_STRATEGY = "DEFAULT_LOAD_BALANCE_STRATEGY";
const std::string PROP_LOAD_BALANCE_STRATEGY = "RR";


const std::string DEFAULT_SERVICE_DISCOVERY_MODE = "sdConfig";
const std::string PROP_SERVICE_DISCOVERY_MODE = "PROP_SERVICE_DISCOVERY_MODE";


const std::string PROP_UNIX_PATH = "PROP_UNIX_PATH";
const std::string DEFAULT_REDIS_UNIX_PATH = "DEFAULT_REDIS_UNIX_PATH";

const std::string  PROP_RECONN_INTERVAL = "PROP_RECONN_INTERVAL";
const std::string  DEFAULT_RECONN_INTERVAL = "5";



