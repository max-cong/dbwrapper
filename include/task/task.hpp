#pragma once

#include "task/evfdClient.hpp"
#include "task/evfdServer.hpp"
#include "task/taskUtil.hpp"

#include "heartbeat/heartbeat.hpp"
#include "connManager/connManager.hpp"
#include "util/dbwType.hpp"
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
namespace task
{
class taskImp : public gene::gene<void *>, public std::enable_shared_from_this<taskImp>, public nonCopyable
{
public:
    explicit taskImp(std::shared_ptr<loop::loop> loopIn) : _connected(false), _evfd(-1), _loop(loopIn)
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
        tmp->process_msg(one);
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
        __LOG(debug, "argv: " << (char *)privdata << ", string is " << reply->str);
        auto ctxSaver = dbw::contextSaver<void *, std::shared_ptr<dbw::redisContext>>::instance();
        auto ctxRet = ctxSaver->getCtx(c);
        if (std::get<1>(ctxRet))
        {
            auto rdxCtx = std::get<0>(ctxRet);
            rdxCtx->_hb->set_hb_success(true);
        }
        else
        {
            // did not find redis context
        }
    }

    static void connectCallback(const redisAsyncContext *c, int status)
    {
        if (status != REDIS_OK)
        {
            __LOG(error, "Error: " << c->errstr);
            return;
        }
        __LOG(debug, "Connected...");
        redisAsyncContext *_aCtx = const_cast<redisAsyncContext *>(c);
        auto ctxSaver = dbw::contextSaver<void *, std::shared_ptr<dbw::redisContext>>::instance();

        std::pair<std::shared_ptr<dbw::redisContext>, bool> contextRet = ctxSaver->getCtx((void *)_aCtx);
        if (std::get<1>(contextRet))
        {
            std::shared_ptr<dbw::redisContext> rdsCtx = std::get<0>(contextRet);

            rdsCtx->_lbs->update_obj(_aCtx, rdsCtx->_priority);
        }
        else
        {

            return;
        }
    }

    static void disconnectCallback(const redisAsyncContext *c, int status)
    {
        if (status != REDIS_OK)
        {
            __LOG(error, "Error: " << c->errstr);
            return;
        }
        __LOG(warn, "Disconnected...\n");
        redisAsyncContext *_aCtx = const_cast<redisAsyncContext *>(c);
        auto ctxSaver = dbw::contextSaver<void *, std::shared_ptr<dbw::redisContext>>::instance();

        auto contextRet = ctxSaver->getCtx(_aCtx);
        if (std::get<1>(contextRet))
        {
            auto rdsCtx = std::get<0>(contextRet);
            rdsCtx->_lbs->del_obj(_aCtx);
        }
        else
        {

            return;
        }
    }

    // set the callback function for evnet coming
    virtual bool on_message(taskMsg const &task_msg)
    {
        bool ret = false;
        switch (task_msg.type)
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

        default:
            __LOG(warn, "unsupport message type!");
            break;
        }
        return ret;
    }
    bool processRedisFormatRawCommand(taskMsg const &task_msg)
    {
        TASK_REDIS_FORMAT_RAW_MSG_BODY msg = DBW_ANY_CAST<TASK_REDIS_FORMAT_RAW_MSG_BODY>(task_msg.body);
        __LOG(debug, "get command :\n"
                         << msg.body);

        auto conn = _connManager->get_conn();
        if (std::get<1>(conn) != lbStrategy::retStatus::SUCCESS)
        {
            __LOG(warn, "did not get connection!");
            return false;
        }
        redisAsyncContext *_context = std::get<0>(conn);

        int ret = redisAsyncFormattedCommand(_context, msg.fn, msg.usr_data, msg.body.c_str(), msg.body.size());
        if (ret != REDIS_OK)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    bool processRedisRawCommand(taskMsg const &task_msg)
    {
        TASK_REDIS_RAW_MSG_BODY msg = DBW_ANY_CAST<TASK_REDIS_RAW_MSG_BODY>(task_msg.body);
        __LOG(debug, "get command :\n"
                         << msg.body);

        auto conn = _connManager->get_conn();
        if (std::get<1>(conn) != lbStrategy::retStatus::SUCCESS)
        {
            __LOG(debug, "did not get a connection");
            return false;
        }

        redisAsyncContext *_context = std::get<0>(conn);
        int ret = redisAsyncCommand(_context, msg.fn, msg.usr_data, msg.body.c_str());
        if (ret != REDIS_OK)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    bool processRedisAddConnection(taskMsg const &task_msg)
    {
        dbw::CONN_INFO payload = DBW_ANY_CAST<dbw::CONN_INFO>(task_msg.body);
        __LOG(error, "connect to : " << payload.ip << ":" << payload.port);

        redisAsyncContext *_context = redisAsyncConnect(payload.ip.c_str(), payload.port);
        if (_context->err)
        {
            __LOG(error, "connect to : " << payload.ip << ":" << payload.port << " return error");
            return false;
        }

        std::shared_ptr<dbw::redisContext> rdsCtx = std::make_shared<dbw::redisContext>();
        rdsCtx->_ctx = _context;
        rdsCtx->_priority = payload.priority;
        rdsCtx->_hb = std::make_shared<heartBeat::heartBeat>(get_loop());

        rdsCtx->_hb->set_genetic_gene(get_genetic_gene());
        rdsCtx->_hb->setPingCb([_context](std::shared_ptr<heartBeat::heartBeat>) {
            __LOG(debug, "now send ping");
            std::string pingMsg("PING");
            redisAsyncCommand(_context, taskImp::pingCallback, (void *)_context, pingMsg.c_str(), pingMsg.size());
        });
        rdsCtx->_hb->init();
        rdsCtx->_lbs = _connManager->getLbs();
        auto ctxSaver = dbw::contextSaver<void *, std::shared_ptr<dbw::redisContext>>::instance();
        ctxSaver->save(_context, rdsCtx);

        int ret = REDIS_ERR;

        ret = redisLibeventAttach(_context, get_loop()->ev());
        if (ret != REDIS_OK)
        {
            __LOG(error, "redisLibeventAttach fail");
            return false;
        }

        redisAsyncSetConnectCallback(_context, connectCallback);
        redisAsyncSetDisconnectCallback(_context, disconnectCallback);
        return true;
    }

    bool processRedisDelConnection(taskMsg const &task_msg)
    {
        // to do
        return false;
    }
    bool init()
    {

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
        __LOG(debug, "init taskImp with ID :" << _evfd);
        // start eventfd server

        _evfdServer = std::make_shared<evfdServer>(get_loop(), _evfd, evfdCallback, (void *)this);
        if (!_evfdServer->init())
        {
            return false;
        }

        // start evfd client

        _evfdClient = std::make_shared<evfdClient>(_evfd);

        _connManager = std::make_shared<connManager::connManager<dbw::CONN_INFO>>(get_loop());
        if (!_connManager)
        {
            return false;
        }
        _connManager->set_genetic_gene(this);

        auto sef_sptr = this->shared_from_this();
        _connManager->setAddConnCb([sef_sptr](dbw::CONN_INFO connInfo) {
            __LOG(debug, "there is a new connection, send message to task with type TASK_REDIS_ADD_CONN");
            return sef_sptr->sendMsg(task::taskMsgType::TASK_REDIS_ADD_CONN, connInfo);
        });
        _connManager->setDecConnCb([sef_sptr](dbw::CONN_INFO connInfo) {
            return sef_sptr->sendMsg(task::taskMsgType::TASK_REDIS_DEL_CONN, connInfo);
        });

        _connManager->setAvaliableCb([sef_sptr]() {
            sef_sptr->_connected = true;
        });
        _connManager->setUnavaliableCb([sef_sptr]() {
            sef_sptr->_connected = false;
        });

        _connManager->init();
        return true;
    }
    void process_msg(uint64_t num)
    {
        for (uint64_t i = 0; i < num; i++)
        {
            auto tmpTaskMsg = _taskQueue.front();
            if (!on_message(tmpTaskMsg))
            {
                // to do start timer?
                sendMsg(tmpTaskMsg);
            }
            _taskQueue.pop();
        }
    }

    bool sendMsg(taskMsg &&msg)
    {
        _taskQueue.emplace(msg);
        if (!_evfdClient)
        {
            return false;
        }
        else
        {
            _evfdClient->send();
        }
        return true;
    }
    bool sendMsg(taskMsg const &msg)
    {
        _taskQueue.emplace(msg);
        if (!_evfdClient)
        {
            return false;
        }
        else
        {
            _evfdClient->send();
        }
        return true;
    }
    bool sendMsg(taskMsgType type, DBW_ANY &&body)
    {
        taskMsg msg;
        msg.type = type;
        msg.body = body;

        return sendMsg(msg);
    }
    bool sendMsgList(std::list<taskMsg> &&msgList)
    {
        if (!_evfdClient)
        {
            return false;
        }
        std::size_t listSize = msgList.size();
        for (auto &&it : msgList)
        {
            _taskQueue.emplace(it);
        }
        _evfdClient->send(listSize);

        return true;
    }

    std::shared_ptr<loop::loop> get_loop()
    {
        return _loop.lock();
    }
    bool getConnStatus()
    {
        return _connected;
    }
    std::atomic<bool> _connected;
    int _evfd;
    std::weak_ptr<loop::loop> _loop;
    std::shared_ptr<evfdClient> _evfdClient;
    std::shared_ptr<evfdServer> _evfdServer;
    TASK_QUEUE _taskQueue;
    std::shared_ptr<connManager::connManager<dbw::CONN_INFO>> _connManager;
};

} // namespace task