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
#include "task/subscribeSet.hpp"

#include "hiredis/async.h"
#include "hiredis/hiredis.h"
#include <string>
#include <memory>
#include <list>
#include <set>
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
static thread_local std::shared_ptr<medis::contextSaver<void *, std::shared_ptr<redisContext>>> tls_ctxSaver;

class taskImp : public gene::gene<void *>, public std::enable_shared_from_this<taskImp>, public nonCopyable
{
public:
    explicit taskImp(std::shared_ptr<loop::loop> loopIn) : _connected(false), _task_q_empty(true), _evfd(-1), _loop(loopIn)
    {
    }
    taskImp() = delete;
    virtual ~taskImp()
    {
        if (CHECK_LOG_LEVEL(warn))
        {
            __LOG(warn, "[taskImp] taskImp is exiting!");
        }
        if (_evfd > 0)
        {
            close(_evfd);
            _evfd = -1;
        }

        _taskQueue.reset();
    }

    void stop()
    {
    }

    bool init()
    {
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "init task with gene : " << (void *)getGeneticGene());
        }

        _evfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (_evfd < 0)
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "!!!!!!!!create event fd fail!");
            }
            return false;
        }
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "init taskImp with ID :" << _evfd);
        }
        // start eventfd server
        if (_loop.expired())
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "loop is invalid!");
            }
            return false;
        }
        _evfdServer = std::make_shared<evfdServer>(getLoop(), _evfd, evfdCallback, (void *)this);
        if (!_evfdServer->init())
        {
            return false;
        }

        // start evfd client

        _evfdClient = std::make_shared<evfdClient>(_evfd);

        // send one message to worker thread.
        std::function<void()> tlsCtxFn = []() {
            tls_ctxSaver = std::make_shared<medis::contextSaver<void *, std::shared_ptr<redisContext>>>();
        };
        sendMsg(taskMsgType::TASK_TLS_CTX_SAVER_INIT, tlsCtxFn);

        _connManagerPubSub = std::make_shared<connManager::connManager<medis::CONN_INFO>>(getLoop());
        if (!_connManagerPubSub)
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "create connection manager for pub subfail");
            }
            return false;
        }
        _connManagerPubSub->setGeneticGene(getGeneticGene());
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "create connection manager with gene : " << _connManagerPubSub->getGeneticGene());
        }

        auto sef_wptr = getThisWptr();

        _connManagerPubSub->setAddConnCb([sef_wptr](std::shared_ptr<medis::CONN_INFO> connInfo) {
            if (!sef_wptr.expired())
            {
                if (CHECK_LOG_LEVEL(debug))
                {
                    __LOG(debug, "there is a new connection, send message to task with type TASK_REDIS_ADD_CONN");
                }
                return sef_wptr.lock()->sendMsg(task::taskMsgType::TASK_REDIS_ADD_CONN_PUB_SUB, connInfo);
            }
            else
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "conn add, task weak ptr is expired");
                }
                return false;
            }
        });
        _connManagerPubSub->setDecConnCb([sef_wptr](std::shared_ptr<medis::CONN_INFO> connInfo) {
            if (!sef_wptr.expired())
            {
                return sef_wptr.lock()->sendMsg(task::taskMsgType::TASK_REDIS_DEL_CONN_PUB_SUB, connInfo);
            }
            else
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "conn del, task weak ptr is expired");
                }
                return false;
            }
        });

        _connManagerPubSub->setAvaliableCb([sef_wptr]() {
            if (!sef_wptr.expired())
            {
                sef_wptr.lock()->setConnStatusPubSub(true);
                // need to re-send pub list
                sef_wptr.lock()->sendMsg(task::taskMsgType::TASK_REDIS_RE_SUB, nullptr);
            }
            else
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "connection avaliable : task weak ptr is expired");
                }
            }
        });
        _connManagerPubSub->setUnavaliableCb([sef_wptr]() {
            if (!sef_wptr.expired())
            {
                sef_wptr.lock()->setConnStatusPubSub(true);
            }

            else
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "connection unavaliable : task weak ptr is expired");
                }
            }
        });

        _connManagerPubSub->init();

        _connManager = std::make_shared<connManager::connManager<medis::CONN_INFO>>(getLoop());
        if (!_connManager)
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "create connection manager fail");
            }
            return false;
        }
        _connManager->setGeneticGene(getGeneticGene());
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "create connection manager with gene : " << _connManager->getGeneticGene());
        }

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
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "conn add, task weak ptr is expired");
                }
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
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "conn del, task weak ptr is expired");
                }
                return false;
            }
        });

        _connManager->setAvaliableCb([sef_wptr]() {
            if (!sef_wptr.expired())
            {
                sef_wptr.lock()->setConnStatus(true);
            }
            else
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "connection avaliable : task weak ptr is expired");
                }
            }
        });
        _connManager->setUnavaliableCb([sef_wptr]() {
            if (!sef_wptr.expired())
            {
                sef_wptr.lock()->setConnStatus(true);
            }
            else
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "connection unavaliable : task weak ptr is expired");
                }
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
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "task weak ptr is expired");
                }
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
            if (CHECK_LOG_LEVEL(warn))
            {
                __LOG(warn, "read return : " << ret);
            }
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
                if (CHECK_LOG_LEVEL(error))
                {
                    __LOG(error, "errstr: " << c->errstr);
                }
            }
            return;
        }
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "argv: " << (char *)privdata << ", string is " << reply->str);
        }

        auto rdxCtx = tls_ctxSaver->getCtx(c).value_or(nullptr);
        if (rdxCtx)
        {
            if (c->err != REDIS_OK)
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "ping has error, error is : " << c->err << ", error string  is : " << std::string(c->errstr));
                }
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
            if (CHECK_LOG_LEVEL(warn))
            {
                __LOG(warn, " did not find redis context");
            }
        }
    }
    static void connectCallbackSimple(const redisAsyncContext *c, int status)
    {
        connectCallback(c, status);
    }
    static void connectCallbackPubSub(const redisAsyncContext *c, int status)
    {
        connectCallback(c, status, true);
    }

    static void connectCallback(const redisAsyncContext *c, int status, bool isSubscribe = false)
    {
        bool connected = false;
        if (status != REDIS_OK || c->err != REDIS_OK)
        {
            connected = false;
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "error code is : " << c->err << ", errstr is : " << c->errstr);
            }
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

        std::shared_ptr<redisContext> rdsCtx = tls_ctxSaver->getCtx((void *)_aCtx).value_or(nullptr);
        if (rdsCtx)
        {
            if (connected)
            {
                rdsCtx->_lbs->updateObj(_aCtx, rdsCtx->_priority);
            }
            else
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "connect return fail, call disconnect callback and disconnect callback will connect again");
                }
                if (isSubscribe)
                {
                    taskImp::disconnectCallbackPubSub(_aCtx, REDIS_OK);
                }
                else
                {
                    taskImp::disconnectCallbackSimple(_aCtx, REDIS_OK);
                }
            }
        }
        else
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "did not find the context!! please check!!");
            }
        }
    }
    static void disconnectCallbackSimple(const redisAsyncContext *c, int status)
    {
        disconnectCallback(c, status);
    }
    static void disconnectCallbackPubSub(const redisAsyncContext *c, int status)
    {
        disconnectCallback(c, status, true);
    }
    static void disconnectCallback(const redisAsyncContext *c, int status, bool isSubscribe = false)
    {
        if (status != REDIS_OK)
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "Error: " << c->errstr);
            }
        }
        if (CHECK_LOG_LEVEL(warn))
        {
            __LOG(warn, "Disconnected... status is :" << status);
        }

        redisAsyncContext *_aCtx = const_cast<redisAsyncContext *>(c);

        auto rdsCtx = tls_ctxSaver->getCtx(_aCtx).value_or(nullptr);
        if (rdsCtx)
        {
            // here delete the obj in the load balancer
            // actually we need to update it, when the new obj comes, then delete it
            // but we do not want to save the old obj info.
            rdsCtx->_lbs->delObj(_aCtx);
            std::string innerIp = rdsCtx->ip;
            unsigned short innerPort = rdsCtx->port;
            int priority = rdsCtx->_priority;
            auto gene = rdsCtx->_hb->getGeneticGene();
            std::weak_ptr<redisContext> ctxWptr(rdsCtx);

            // give sub list back to glob sub list
            if (isSubscribe)
            {
                auto task_sptr = medis::taskSaver<void *, std::shared_ptr<task::taskImp>>::instance()->getTask(gene).value_or(nullptr);
                if (task_sptr)
                {
                    task_sptr->_subsSet.update(_aCtx);
                    // send a TASK_REDIS_RE_SUB,  other connection will take the sub info
                    // if there is no connection, this  will covered by on_avaliable
                    task_sptr->sendMsg(task::taskMsgType::TASK_REDIS_RE_SUB, nullptr);
                }
            }

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
                // remove the related info in the context saver

                tls_ctxSaver->del(_aCtx);
            });
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
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "message in the task queue is invalid");
            }
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
        case taskMsgType::TASK_REDIS_ADD_CONN_PUB_SUB:
            ret = processRedisAddConnection(task_msg, true);
            break;
        case taskMsgType::TASK_REDIS_DEL_CONN:
            ret = processRedisDelConnection(task_msg);
            break;
        case taskMsgType::TASK_REDIS_DEL_CONN_PUB_SUB:
            ret = processRedisDelConnection(task_msg, true);
            break;
        case taskMsgType::TASK_REDIS_CONN_AVALIABLE:
            ret = processRedisConnAvaliable();
            break;

        case taskMsgType::TASK_TLS_CTX_SAVER_INIT:
            ret = processTlsCtxSaver(task_msg);
            break;

        case taskMsgType::TASK_REDIS_RE_SUB:
            ret = processReSend();
            break;

        default:
            if (CHECK_LOG_LEVEL(warn))
            {
                __LOG(warn, "unsupport message type!");
            }
            break;
        }
        return ret;
    }
    bool processReSend()
    {
        auto toSub = _subsSet.getToSub();

        for (auto it : toSub)
        {
            on_message(it.first);
        }
        return true;
    }
    bool processTlsCtxSaver(std::shared_ptr<taskMsg> task_msg)
    {
        if (!task_msg)
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "invalid task message!");
            }
        }
        if (CHECK_LOG_LEVEL(warn))
        {
            __LOG(warn, "set tls context saver!");
        }
        std::function<void()> fn = DBW_ANY_CAST<std::function<void()>>(task_msg->body);
        fn();
        return true;
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
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "invalid task message!");
            }
        }
        std::shared_ptr<TASK_REDIS_FORMAT_RAW_MSG_BODY> msg = DBW_ANY_CAST<std::shared_ptr<TASK_REDIS_FORMAT_RAW_MSG_BODY>>(task_msg->body);
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "get command :\n"
                             << msg->body);
        }
        redisAsyncContext *_context = NULL;

        switch (msg->cmdType)
        {
        case REDIS_COMMAND_TYPE::TASK_REDIS_SUB:
            // need to create relationship between redisAsyncContext and the related message.
            {
                auto setPair = _subsSet.get(msg->body).value_or(std::make_pair(nullptr, (redisAsyncContext*)NULL));
                if (setPair.second)
                {
                    // if you had sub before. then use the context
                    // hiredis will use the new userdate and callback function
                    _context = setPair.second;
                }
                else
                {
                    // if not. get a new context. then get a new one
                    _context = (redisAsyncContext *)(_connManagerPubSub->get_conn()).value_or(nullptr);
                    _subsSet.update(task_msg, _context);
                }
            }
            break;
        case REDIS_COMMAND_TYPE::TASK_REDIS_UNSUB:
            // need to delete the record in the subscribe set.
            {

                auto setPair = _subsSet.get(msg->body).value_or(std::make_pair(nullptr, (redisAsyncContext*)NULL));
                if (setPair.second)
                {
                    if (setPair.second)
                    {
                        // if there is a record in the subscribe set, then delete it using the redisAsyncContext.
                        _context = setPair.second;
                    }
                    else
                    {
                        // if there is a record in the subscribe set, but the redisAsyncContext is not valid. then do nothing(maybe rainyday case?)
                        if (CHECK_LOG_LEVEL(debug))
                        {
                            __LOG(debug, "there is record, but there is no redisAsyncContext, command is " << msg->body);
                        }
                    }
                }
                else
                {
                    // if there is no record, then do nothing
                    if (CHECK_LOG_LEVEL(debug))
                    {
                        __LOG(debug, "there is no record in the record set, command is : " << msg->body);
                    }
                }
                _subsSet.del(msg->body);
                if (!_context)
                {
                    if (CHECK_LOG_LEVEL(warn))
                    {
                        __LOG(warn, "did not get connection in the sub set. maybe not subscribe before!");
                    }
                    return false;
                }
            }
            break;
        default:
            _context = (redisAsyncContext *)(_connManager->get_conn()).value_or(nullptr);
            break;
        }

        if (!_context)
        {
            if (CHECK_LOG_LEVEL(warn))
            {
                __LOG(warn, "did not get connection!");
            }
            return false;
        }

        int ret = redisAsyncFormattedCommand(_context, msg->fn, msg->usr_data, msg->body.c_str(), msg->body.size());
        if (ret != REDIS_OK)
        {
            if (CHECK_LOG_LEVEL(warn))
            {
                __LOG(warn, "send message return fail, check the connection!");
            }
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
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "invalid task message!");
            }
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
            if (CHECK_LOG_LEVEL(warn))
            {
                __LOG(warn, "did not get connection!");
            }
            return false;
        }

        int ret = redisAsyncCommand(_context, msg->fn, msg->usr_data, msg->body.c_str());
        if (ret != REDIS_OK)
        {
            if (CHECK_LOG_LEVEL(warn))
            {
                __LOG(warn, "send message return fail, check the connection!");
            }
            return false;
        }
        else
        {
            return true;
        }
    }
    bool processRedisAddConnection(std::shared_ptr<taskMsg> task_msg, bool isSubscribe = false)
    {
        if (!task_msg)
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "invalid task message!");
            }
        }
        std::shared_ptr<medis::CONN_INFO> connInfo_sptr = DBW_ANY_CAST<std::shared_ptr<medis::CONN_INFO>>(task_msg->body);
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "connect to : " << connInfo_sptr->ip << ":" << connInfo_sptr->port);
        }

        redisAsyncContext *_context = redisAsyncConnect(connInfo_sptr->ip.c_str(), connInfo_sptr->port);
        if (_context->err != REDIS_OK)
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "connect to : " << connInfo_sptr->ip << ":" << connInfo_sptr->port << " return error");
            }
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
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "the redis context is expired!");
                }
            }
        });

        rdsCtx->_hb->setHbLostCb([rdsCtx_wptr, isSubscribe]() {
            if (!rdsCtx_wptr.expired())
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "heart beat lost, call disconnect callback");
                }
                auto rdsCtx = rdsCtx_wptr.lock();
                if (isSubscribe)
                {
                    taskImp::disconnectCallbackPubSub(rdsCtx->_ctx, REDIS_OK);
                }
                else
                {
                    taskImp::disconnectCallbackSimple(rdsCtx->_ctx, REDIS_OK);
                }
            }
            else
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "the redis context is expired!");
                }
            }
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
        if (!isSubscribe)
        {
            rdsCtx->_lbs = _connManager->getLbs();
        }
        else
        {
            rdsCtx->_lbs = _connManagerPubSub->getLbs();
        }

        tls_ctxSaver->save(_context, rdsCtx);

        int ret = REDIS_ERR;

        ret = redisLibeventAttach(_context, getLoop()->ev());
        if (ret != REDIS_OK)
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "redisLibeventAttach fail");
            }
            return false;
        }
        if (isSubscribe)
        {
            redisAsyncSetConnectCallback(_context, connectCallbackPubSub);
            redisAsyncSetDisconnectCallback(_context, disconnectCallbackPubSub);
        }
        else
        {
            redisAsyncSetConnectCallback(_context, connectCallbackSimple);
            redisAsyncSetDisconnectCallback(_context, disconnectCallbackSimple);
        }

        return true;
    }

    bool processRedisDelConnection(std::shared_ptr<taskMsg> task_msg, bool isSubscribe = false)
    {
        if (!task_msg)
        {
            if (CHECK_LOG_LEVEL(error))
            {
                __LOG(error, "invalid task message!");
            }
        }
        std::shared_ptr<medis::CONN_INFO> connInfo_sptr = DBW_ANY_CAST<std::shared_ptr<medis::CONN_INFO>>(task_msg->body);
        if (CHECK_LOG_LEVEL(debug))
        {
            __LOG(debug, "disconnect to : " << connInfo_sptr->ip << ":" << connInfo_sptr->port);
        }

        auto ctxList = tls_ctxSaver->getIpPortThenDel(connInfo_sptr->ip, connInfo_sptr->port);
        for (auto it : ctxList)
        {
            // need to delete related info from load balancer
            if (it->_lbs->delObj(it->_ctx) == medis::retStatus::SUCCESS)
            {
                // need to stop the hiredis connection here
                redisAsyncDisconnect(it->_ctx);
                if (isSubscribe)
                {
                    // if this is a pub/sub connection, should add the related pub msg into the golb to be publish list
                    _subsSet.update(it->_ctx);
                }
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
                if (CHECK_LOG_LEVEL(warn))
		{__LOG(warn,"task process message, task weak ptr is expired");}
                return;
            }
            */
            if (!on_message(tmpTaskMsg))
            {
                if (CHECK_LOG_LEVEL(warn))
                {
                    __LOG(warn, "process task message return fail!");
                }
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
                        if (CHECK_LOG_LEVEL(warn))
                        {
                            __LOG(warn, "task process message, task weak ptr is expired");
                        }
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
    void setConnStatus(bool medisState)
    {
        _connected = medisState;
    }
    bool getConnStatusPubSub()
    {
        return _connectedPubSub;
    }
    void setConnStatusPubSub(bool medisState)
    {
        _connectedPubSub = medisState;
    }
    std::weak_ptr<taskImp> getThisWptr()
    {
        std::weak_ptr<taskImp> self_wptr(shared_from_this());
        return self_wptr;
    }
    std::atomic<bool> _connected;
    std::atomic<bool> _connectedPubSub;
    std::atomic<bool> _task_q_empty;
    int _evfd;
    std::weak_ptr<loop::loop> _loop;
    std::shared_ptr<evfdClient> _evfdClient;
    std::shared_ptr<evfdServer> _evfdServer;
    TASK_QUEUE _taskQueue;
    std::shared_ptr<connManager::connManager<medis::CONN_INFO>> _connManager;
    std::shared_ptr<connManager::connManager<medis::CONN_INFO>> _connManagerPubSub;
    std::shared_ptr<timer::timerManager> _timerManager;
    subscribeSet _subsSet;
};

} // namespace task
