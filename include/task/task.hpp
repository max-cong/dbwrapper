#pragma once
namespace task
{

class task : public gene::gene
{
public:
    task(std::shared_ptr<Loop> loopIn) : _loop(loopIn), _evfd(-1)
    {
    }
    task() = delete;
    virtual ~task()
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
        task *tmp = reinterpret_cast<task *>(args);
        tmp->process_msg(one);
    }
    static void pingCallback(redisAsyncContext *c, void *r, void *privdata)
    {
        redisReply *reply = r;
        if (reply == NULL)
        {
            if (c->errstr)
            {
                printf("errstr: %s\n", c->errstr);
            }
            return;
        }
        printf("argv[%s]: %s\n", (char *)privdata, reply->str);
        auto ctxSaver = contextSaver::instance();
        auto ctxRet = ctxSaver->get(_context);
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

    // set the callback function for evnet coming
    virtual bool on_message(taskMsg msg)
    {
        switch (task_msg.type)
        {
        case taskMsgType::TASK_REDIS_FORMAT_RAW:
        {
            std::string msg = DBW_ANY_CAST<std::string>(task_msg.body);
            __LOG(debug, "get command :\n"
                             << msg.body);
            redisAsyncFormattedCommand(_context, msg.cb, msg.usr_data, msg.body.c_str(), msg.body.size());
            break;
        }

        case taskMsgType::TASK_REDIS_RAW:
        {
            TASK_REDIS_RAW_MSG msg = DBW_ANY_CAST<TASK_REDIS_RAW_MSG>(task_msg.body);
            __LOG(debug, "get command :\n"
                             << msg.body);
            redisAsyncCommand(_context, msg.cb, msg.usr_data, msg.body.c_str());
            break;
        }

        case taskMsgType::TASK_REDIS_ADD_CONN:
        {
            add_conn_payload payload = DBW_ANY_CAST<add_conn_payload>(task_msg.body);
            __LOG(error, "connect to : " << payload.ip << ":" << payload.port);

            redisAsyncContext *_context = redisAsyncConnect(payload.ip.c_str(), payload.port);
            if (_context->err)
            {
                __LOG(error, "connect to : " << payload.ip << ":" << payload.port << " return error");
                return false;
            }

            std::shared_ptr<redisContext> rdsCtx = std::make_shared<redisContext>();
            rdsCtx->_ctx = _context;
            rdsCtx->_hb = std::make_shared<heartBeat::heartBeat>(get_loop());
            rdsCtx->_hb->set_genetic_gene(get_genetic_gene());
            rdsCtx->_hb->setPingCb([_context](std::shared_ptr<heartBeat>) {
                std::string pingMsg("PING");
                redisAsyncCommand(_context, task::pingCallback, (void *)_context, pingMsg.c_str(), pingMsg.size());
            });
            rdsCtx->_lbs = _connManager->getLbs();
            auto ctxSaver = contextSaver::instance();
            ctxSaver->save(_context, rdsCtx);

            redisLibeventAttach(_context, get_loop());

            redisAsyncSetConnectCallback(_context, _connect_cb);
            redisAsyncSetDisconnectCallback(_context, _disconnect_cb);
            break;
        }
        case taskMsgType::TASK_REDIS_DEL_CONN:
        {
            break;
        }
        default:
            __LOG(warn, "unsupport message type!");
            break;
        }
        return true;
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
        __LOG(debug, "init task with ID :" << _evfd);
        // start eventfd server
        try
        {
            _evfdServer = std::make_shared<task::evfdServer>(get_loop())->ev(), _evfd, evfdCallback, this);
        }
        catch (std::exception &e)
        {
            __LOG(error, "!!!!!!!!!!!!exception happend when trying to create event fd server, info :" << e.what());
            return false;
        }
        // start evfd client
        try
        {
            _evfdClient = std::make_shared<task::evfdClient>(_evfd);
        }
        catch (std::exception &e)
        {
            __LOG(error, "!!!!!!!!!!!!exception happend when trying to create event fd server, info :" << e.what());
            return false;
        }
        _connManager = std::make_shared<connManager<redisAsyncContext *>>(get_loop());
        if (_connManager)
        {
            return false;
        }
        return true;
    }
    void process_msg(uint64_t num)
    {
        for (int i = 0; i < num; i++)
        {
            auto tmpTaskMsg = _taskQueue.front();
            on_message(tmpTaskMsg);
            _taskQueue.pop();
        }
    }

    bool sendMsg(TASK_MSG &&msg)
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
    bool sendMsgList(std::list<TASK_MSG> &&msgList)
    {
        if (!_evfdClient)
        {
            return false;
        }
        for (auto &&it : msgList)
        {
            _taskQueue.emplace(msg);
        }
        _evfdClient->send(msgList.size());

        return true;
    }

    std::shared_ptr<loop::loop> get_loop()
    {
        return _loop.lock();
    }

    int _evfd;
    std::weak_ptr<loop::loop> _loop;
    std::shared_ptr<task::evfdClient> _evfdClient;
    std::shared_ptr<task::evfdServer> _evfdServer;
    TASK_QUEUE _taskQueue;
    std::shared_ptr<connManager<redisAsyncContext *>> _connManager;
};

} // namespace task