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

#include "task/evfdClient.hpp"
#include "task/evfdServer.hpp"
#include "task/taskUtil.hpp"

#include "heartbeat/heartbeat.hpp"
#include "connManager/connManager.hpp"
#include "util/medisType.hpp"
#include "util/defs.hpp"
#include "gene/gene.hpp"
#include "loop/loop.hpp"
#include "util/nonCopyable.hpp"

#include "hiredis/async.h"
#include "hiredis/hiredis.h"
#include <string>
#include <memory>
#include <list>
#include <tuple>
#include <unistd.h>
#include <sys/eventfd.h>
#include <atomic>
#include <arpa/inet.h>

namespace task
{
struct redisContext
{
    int _priority;
    std::string ip;
    unsigned short port;
    redisAsyncContext *_ctx;
    std::shared_ptr<heartBeat::heartBeat> _hb;
    std::shared_ptr<lbStrategy::lbStrategy<redisAsyncContext *>> _lbs;
    std::shared_ptr<timer::timerManager> _retryTimerManager;
};
class taskImp : public gene::gene<void *>, public std::enable_shared_from_this<taskImp>, public nonCopyable
{
public:
    explicit taskImp(std::shared_ptr<loop::loop> loopIn) : _connected(false), _task_q_empty(true), _evfd(-1), _loop(loopIn)
    {
    }
    taskImp() = delete;
    virtual ~taskImp()
    {
        if (_evfd > 0)
        {
            close(_evfd);
            _evfd = -1;
        }
    }
    bool init()
    {
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "init task with gene : " << (void *)getGeneticGene());
        }
        if (_loop.expired())
        {
            __LOG(error, "loop is invalid!");
            return false;
        }
        _evfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (_evfd < 0)
        {
            __LOG(error, "!!!!!!!!create event fd fail!");
            return false;
        }
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "init taskImp with ID :" << _evfd);
        }
        // start eventfd server

        _evfdServer = std::make_shared<evfdServer>(getLoop(), _evfd, evfdCallback, (void *)this);
        if (!_evfdServer->init())
        {
            return false;
        }

        // start evfd client

        _evfdClient = std::make_shared<evfdClient>(_evfd);

        _connManager = std::make_shared<connManager::connManager<medis::CONN_INFO>>(getLoop());
        if (!_connManager)
        {
            __LOG(error, "create connection manager fail");
            return false;
        }
        _connManager->setGeneticGene(getGeneticGene());
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "create connection manager with gene : " << _connManager->getGeneticGene());
        }

        auto sef_wptr = getThisWptr();

        _connManager->setAddConnCb([sef_wptr](std::shared_ptr<medis::CONN_INFO> connInfo) {
            if (!sef_wptr.expired())
            {
                if (CHECK_LOG_LEVEL(debug))
                {
                    __LOG(debug, "there is a new connection, send message to task with type TASK_REDIS_ADD_CONN");
                }
                return sef_wptr.lock()->sendMsg(task::taskMsgType::TASK_REDIS_ADD_CONN, connInfo);
            }
            else
            {
                __LOG(warn, "conn add, task weak ptr is expired");
                return false;
            }
        });
        _connManager->setDecConnCb([sef_wptr](std::shared_ptr<medis::CONN_INFO> connInfo) {
            if (!sef_wptr.expired())
            {
                return sef_wptr.lock()->sendMsg(task::taskMsgType::TASK_REDIS_DEL_CONN, connInfo);
            }
            else
            {
                __LOG(warn, "conn del, task weak ptr is expired");
                return false;
            }
        });

        _connManager->setAvaliableCb([sef_wptr]() {
            if (!sef_wptr.expired())
            {
                sef_wptr.lock()->_connected = true;
            }
            else
            {
                __LOG(warn, "connection avaliable : task weak ptr is expired");
            }
        });
        _connManager->setUnavaliableCb([sef_wptr]() {
            if (!sef_wptr.expired())
            {
                sef_wptr.lock()->_connected = false;
            }
            else
            {
                __LOG(warn, "connection unavaliable : task weak ptr is expired");
            }
        });

        _connManager->init();
        _timerManager.reset(new timer::timerManager(getLoop()));
        // start a 100ms timer to guard A-B-A issue.

        _timerManager->getTimer()->startForever(100, [sef_wptr]() {
            if (!sef_wptr.expired())
            {
                std::shared_ptr<taskImp> self_sptr = sef_wptr.lock();
                if (self_sptr->_taskQueue.read_available() > 0)
                {
                    self_sptr->process_msg();
                }
            }
            else
            {
                __LOG(warn, "task weak ptr is expired");
            }
        });
        return true;
    }
    static void evfdCallback(int fd, short event, void *args)
    {
        uint64_t one;
        int ret = read(fd, &one, sizeof one);
        if (ret != sizeof one)
        {
            __LOG(warn, "read return : " << ret);
            return;
        }
        taskImp *tmp = reinterpret_cast<taskImp *>(args);
        tmp->process_msg();
    }
    static void pingCallback(redisAsyncContext *c, void *r, void *privdata)
    {
        redisReply *reply = (redisReply *)r;
        if (reply == NULL)
        {
            if (c->errstr)
            {
                __LOG(error, "errstr: " << c->errstr);
            }
            return;
        }
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "argv: " << (char *)privdata << ", string is " << reply->str);
        }

        auto ctxSaver = medis::contextSaver<void *, std::shared_ptr<redisContext>>::instance();
        auto rdxCtx = ctxSaver->getCtx(c).value_or(nullptr);
        if (rdxCtx)
        {
            if (c->err != REDIS_OK)
            {
                __LOG(warn, "ping has error, error is : " << c->err << ", error string  is : " << std::string(c->errstr));
                rdxCtx->_hb->setHbSuccess(false);
            }
            else
            {
                if (CHECK_LOG_LEVEL(debug))
                {
                    __LOG(debug, "ping success");
                }

                rdxCtx->_hb->setHbSuccess(true);
            }
        }
        else
        {
            // did not find redis context
            __LOG(warn, " did not find redis context");
        }
    }

    static void connectCallback(const redisAsyncContext *c, int status)
    {
        bool connected = false;
        if (status != REDIS_OK || c->err != REDIS_OK)
        {
            connected = false;
            __LOG(error, "error code is : " << c->err << ", errstr is : " << c->errstr);
        }
        else
        {
            connected = true;
            if (CHECK_LOG_LEVEL(debug))
            {
                __LOG(debug, "Connected...");
            }
        }

        redisAsyncContext *_aCtx = const_cast<redisAsyncContext *>(c);
        auto ctxSaver = medis::contextSaver<void *, std::shared_ptr<redisContext>>::instance();

        std::shared_ptr<redisContext> rdsCtx = ctxSaver->getCtx((void *)_aCtx).value_or(nullptr);
        if (rdsCtx)
        {
            if (connected)
            {
                rdsCtx->_lbs->updateObj(_aCtx, rdsCtx->_priority);
            }
            else
            {
                __LOG(warn, "connect return fail, call disconnect callback and disconnect callback will connect again");
                taskImp::disconnectCallback(_aCtx, REDIS_OK);
            }
        }
        else
        {
            __LOG(error, "did not find the context!! please check!!");
        }
    }

    static void disconnectCallback(const redisAsyncContext *c, int status)
    {
        if (status != REDIS_OK)
        {
            __LOG(error, "Error: " << c->errstr);
        }
        __LOG(warn, "Disconnected... status is :" << status);
        redisAsyncContext *_aCtx = const_cast<redisAsyncContext *>(c);
        auto ctxSaver = medis::contextSaver<void *, std::shared_ptr<redisContext>>::instance();

        auto rdsCtx = ctxSaver->getCtx(_aCtx).value_or(nullptr);
        if (rdsCtx)
        {
            rdsCtx->_lbs->delObj(_aCtx);
            std::string innerIp = rdsCtx->ip;
            unsigned short innerPort = rdsCtx->port;
            int priority = rdsCtx->_priority;
            auto gene = rdsCtx->_hb->getGeneticGene();
            std::weak_ptr<redisContext> ctxWptr(rdsCtx);

            // get reconnect timer config
            std::string reconnectItvalStr = configCenter::configCenter<void *>::instance()->getPropertiesField(gene, PROP_RECONN_INTERVAL, DEFAULT_HB_INTERVAL);
            std::string::size_type sz; // alias of size_t
            int _interval = std::stoi(reconnectItvalStr, &sz) * 1000;
            if (CHECK_LOG_LEVEL(debug))
            {
                __LOG(debug, "now start reconnect timer [" << _interval << "ms]");
            }
            rdsCtx->_retryTimerManager->getTimer()->startOnce(_interval, [gene, innerIp, innerPort, priority, _aCtx, ctxWptr]() {
                if (CHECK_LOG_LEVEL(debug))
                {
                    __LOG(debug, "in the reconnect timer");
                }
                auto task_sptr = medis::taskSaver<void *, std::shared_ptr<task::taskImp>>::instance()->getTask(gene).value_or(nullptr);

                if (task_sptr)
                {
                    std::shared_ptr<medis::CONN_INFO> connInfo_sptr;
                    connInfo_sptr->ip = innerIp;
                    connInfo_sptr->port = innerPort;
                    connInfo_sptr->priority = priority;
                    if (!ctxWptr.expired())
                    {
                        connInfo_sptr->hbTime = ctxWptr.lock()->_hb->getRetryNum();
                        if (CHECK_LOG_LEVEL(debug))
                        {
                            __LOG(debug, "heartbeat retry time left is : " << connInfo_sptr->hbTime);
                        }
                    }

                    task_sptr->sendMsg(taskMsgType::TASK_REDIS_ADD_CONN, connInfo_sptr);
                }
                auto ctxSaver = medis::contextSaver<void *, std::shared_ptr<redisContext>>::instance();
                ctxSaver->del(_aCtx);
            });
            // remove the related info in the context saver
        }
        else
        {
            if (CHECK_LOG_LEVEL(debug))
            {
                __LOG(debug, "did not get the context info in the context saver, context is : " << (void *)_aCtx);
            }
        }
        // reconnect
    }

    // set the callback function for evnet coming
    virtual bool on_message(std::shared_ptr<taskMsg> task_msg)
    {
        if (!task_msg)
        {
            __LOG(error, "message in the task queue is invalid");
        }
        bool ret = false;
        switch (task_msg->type)
        {
        case taskMsgType::TASK_REDIS_FORMAT_RAW:
            ret = processRedisFormatRawCommand(task_msg);
            break;

        case taskMsgType::TASK_REDIS_RAW:
            ret = processRedisRawCommand(task_msg);
            break;

        case taskMsgType::TASK_REDIS_ADD_CONN:

            ret = processRedisAddConnection(task_msg);
            break;

        case taskMsgType::TASK_REDIS_DEL_CONN:
            ret = processRedisDelConnection(task_msg);
            break;

        case taskMsgType::TASK_REDIS_CONN_AVALIABLE:
            ret = processRedisConnAvaliable();
            break;

        default:
            __LOG(warn, "unsupport message type!");
            break;
        }
        return ret;
    }

    bool processRedisConnAvaliable()
    {
        // to do
        return false;
    }
    bool processRedisFormatRawCommand(std::shared_ptr<taskMsg> task_msg)
    {
        if (!task_msg)
        {
            __LOG(error, "invalid task message!");
        }
        std::shared_ptr<TASK_REDIS_FORMAT_RAW_MSG_BODY> msg = DBW_ANY_CAST<std::shared_ptr<TASK_REDIS_FORMAT_RAW_MSG_BODY>>(task_msg->body);
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "get command :\n"
                             << msg->body);
        }

        redisAsyncContext *_context = (redisAsyncContext *)(_connManager->get_conn()).value_or(nullptr);
        if (!_context)
        {
            __LOG(warn, "did not get connection!");
            return false;
        }

        int ret = redisAsyncFormattedCommand(_context, msg->fn, msg->usr_data, msg->body.c_str(), msg->body.size());
        if (ret != REDIS_OK)
        {
            __LOG(warn, "send message return fail, check the connection!");
            return false;
        }
        else
        {
            return true;
        }
    }
    bool processRedisRawCommand(std::shared_ptr<taskMsg> task_msg)
    {
        if (!task_msg)
        {
            __LOG(error, "invalid task message!");
        }
        std::shared_ptr<TASK_REDIS_RAW_MSG_BODY> msg = DBW_ANY_CAST<std::shared_ptr<TASK_REDIS_RAW_MSG_BODY>>(task_msg->body);
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "get command :\n"
                             << msg->body);
        }

        redisAsyncContext *_context = (_connManager->get_conn()).value_or(nullptr);
        if (!_context)
        {
            __LOG(warn, "did not get connection!");
            return false;
        }

        int ret = redisAsyncCommand(_context, msg->fn, msg->usr_data, msg->body.c_str());
        if (ret != REDIS_OK)
        {
            __LOG(warn, "send message return fail, check the connection!");
            return false;
        }
        else
        {
            return true;
        }
    }
    bool processRedisAddConnection(std::shared_ptr<taskMsg> task_msg)
    {
        if (!task_msg)
        {
            __LOG(error, "invalid task message!");
        }
        std::shared_ptr<medis::CONN_INFO> connInfo_sptr = DBW_ANY_CAST<std::shared_ptr<medis::CONN_INFO>>(task_msg->body);
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "connect to : " << connInfo_sptr->ip << ":" << connInfo_sptr->port);
        }

        redisAsyncContext *_context = redisAsyncConnect(connInfo_sptr->ip.c_str(), connInfo_sptr->port);
        if (_context->err != REDIS_OK)
        {
            __LOG(error, "connect to : " << connInfo_sptr->ip << ":" << connInfo_sptr->port << " return error");
            return false;
        }

        std::shared_ptr<redisContext> rdsCtx = std::make_shared<redisContext>();
        std::weak_ptr<redisContext> rdsCtx_wptr(rdsCtx);
        rdsCtx->_ctx = _context;
        rdsCtx->_priority = connInfo_sptr->priority;
        rdsCtx->ip = connInfo_sptr->ip;
        rdsCtx->port = connInfo_sptr->port;
        rdsCtx->_hb = std::make_shared<heartBeat::heartBeat>(getLoop());
        rdsCtx->_hb->setGeneticGene(getGeneticGene());
        if (connInfo_sptr->hbTime)
        { // if there is heartbeat info, set accordingly
            rdsCtx->_hb->setRetryNum(connInfo_sptr->hbTime);
        }
        rdsCtx->_hb->setPingCb([rdsCtx_wptr]() {
            if (!rdsCtx_wptr.expired())
            {
                auto rdsCtx = rdsCtx_wptr.lock();
                if (CHECK_LOG_LEVEL(debug))
                {
                    __LOG(debug, "now send ping");
                }
                std::string pingMsg("PING");
                redisAsyncCommand(rdsCtx->_ctx, taskImp::pingCallback, (void *)(rdsCtx->_ctx), pingMsg.c_str(), pingMsg.size());
            }
            else
            {
                __LOG(warn, "the redis context is expired!");
            }
        });
        std::weak_ptr<connManager::connManager<medis::CONN_INFO>> _connManager_wptr(_connManager);
        rdsCtx->_hb->setHbLostCb([rdsCtx_wptr, _connManager_wptr]() {
            if (!rdsCtx_wptr.expired())
            {
                __LOG(warn, "heart beat lost, call disconnect callback");
                auto rdsCtx = rdsCtx_wptr.lock();
                taskImp::disconnectCallback(rdsCtx->_ctx, REDIS_OK);
            }
            else
            {
                __LOG(warn, "the redis context is expired!");
            }
            // call connection manager onUnavaliable
            /*
            if (!_connManager_wptr.expired())
            {
                __LOG(warn, "there is no avaliable connection");
                _connManager_wptr.lock()->onUnavaliable();
            }*/
        });
        rdsCtx->_hb->setHbSuccCb([rdsCtx_wptr]() {
            if (!rdsCtx_wptr.expired())
            {
                auto rdsCtx = rdsCtx_wptr.lock();
                if (CHECK_LOG_LEVEL(debug))
                {
                    __LOG(debug, "heartbeat success on context : " << (void *)rdsCtx->_ctx << "(note: maybe this is the first timer hb. if this log happens more than once, it means success)");
                }
            }
        });
        rdsCtx->_hb->init();
        rdsCtx->_retryTimerManager = std::make_shared<timer::timerManager>(getLoop());
        rdsCtx->_lbs = _connManager->getLbs();

        auto ctxSaver = medis::contextSaver<void *, std::shared_ptr<redisContext>>::instance();
        ctxSaver->save(_context, rdsCtx);

        int ret = REDIS_ERR;

        ret = redisLibeventAttach(_context, getLoop()->ev());
        if (ret != REDIS_OK)
        {
            __LOG(error, "redisLibeventAttach fail");
            return false;
        }

        redisAsyncSetConnectCallback(_context, connectCallback);
        redisAsyncSetDisconnectCallback(_context, disconnectCallback);
        return true;
    }

    bool processRedisDelConnection(std::shared_ptr<taskMsg> task_msg)
    {
        if (!task_msg)
        {
            __LOG(error, "invalid task message!");
        }
        std::shared_ptr<medis::CONN_INFO> connInfo_sptr = DBW_ANY_CAST<std::shared_ptr<medis::CONN_INFO>>(task_msg->body);
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "disconnect to : " << connInfo_sptr->ip << ":" << connInfo_sptr->port);
        }

        auto ctxSaver = medis::contextSaver<void *, std::shared_ptr<redisContext>>::instance();
        auto ctxList = ctxSaver->getIpPortThenDel(connInfo_sptr->ip, connInfo_sptr->port);
        for (auto it : ctxList)
        {
            // need to delete related info from load balancer
            if (it->_lbs->delObj(it->_ctx) == medis::retStatus::SUCCESS)
            {
                // need to stop the hiredis connection here
                redisAsyncDisconnect(it->_ctx);
            }
            else
            {
                // there is no connection in the lb
                if (CHECK_LOG_LEVEL(debug))
                {
                    __LOG(debug, "there is not connection in the connection mananger");
                }
            }
        }
        return true;
    }

    void process_msg()
    {
        _task_q_empty = _taskQueue.read_available() > 0 ? false : true;
        //auto self_wptr = getThisWptr();
        //auto self_sptr = shared_from_this();
        _taskQueue.consume_all([this](std::shared_ptr<taskMsg> tmpTaskMsg) {
            /*
            std::shared_ptr<taskImp> self_sptr;
            if (!self_wptr.expired())
            {
                self_sptr = self_wptr.lock();
            }
            else
            {
                __LOG(warn, "task process message, task weak ptr is expired");
                return;
            }
            */
            if (!on_message(tmpTaskMsg))
            {
                __LOG(warn, "process task message return fail!");
                // the message process return fail
                // start a timer to send message to task again

                auto self_wptr = getThisWptr();
                _timerManager->getTimer()->startOnce(100, [self_wptr, tmpTaskMsg]() {
                    if (!self_wptr.expired())
                    {
                        self_wptr.lock()->sendMsg(tmpTaskMsg);
                    }
                    else
                    {
                        __LOG(warn, "task process message, task weak ptr is expired");
                    }
                });
            }
        });
        // to do A-B-A issue
        _task_q_empty = true;
    }

    bool sendMsg(std::shared_ptr<taskMsg> msg)
    {
        // to do , now just make sure sent the message

        while (!_taskQueue.push(msg))
        {
        }

        if (!_evfdClient)
        {
            return false;
        }
        else
        {
            if (_task_q_empty)
            {
                _evfdClient->send();
            }
        }
        return true;
    }

    bool sendMsg(taskMsgType type, DBW_ANY &&body)
    {
        std::shared_ptr<taskMsg> msg_sptr = std::make_shared<taskMsg>();
        msg_sptr->type = type;
        msg_sptr->body = body;

        return sendMsg(msg_sptr);
    }

    std::shared_ptr<loop::loop> getLoop()
    {
        return _loop.lock();
    }
    bool getConnStatus()
    {
        return _connected;
    }

    std::weak_ptr<taskImp> getThisWptr()
    {
        std::weak_ptr<taskImp> self_wptr(shared_from_this());
        return self_wptr;
    }
    std::atomic<bool> _connected;
    std::atomic<bool> _task_q_empty;
    int _evfd;
    std::weak_ptr<loop::loop> _loop;
    std::shared_ptr<evfdClient> _evfdClient;
    std::shared_ptr<evfdServer> _evfdServer;
    TASK_QUEUE _taskQueue;
    std::shared_ptr<connManager::connManager<medis::CONN_INFO>> _connManager;
    std::shared_ptr<timer::timerManager> _timerManager;
};

} // namespace task
