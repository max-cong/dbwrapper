#pragma once
namespace task
{

class task
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

    // set the callback function for evnet coming
    virtual bool on_message(TASK_MSG msg) = 0;
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
            _evfdServer = std::make_shared<task::evfdServer>((_loop.lock())->ev(), _evfd, evfdCallback, this);
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
        for (auto&& it : msgList)
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
};

} // namespace task