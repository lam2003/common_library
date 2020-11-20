#include <net/socket_utils.h>
#include <poller/event_poller.h>
#include <utils/time_ticker.h>
#include <utils/utils.h>
#include <utils/uv_error.h>

#include <sys/epoll.h>
#include <unistd.h>

#define EPOLL_SIZE 1024  // 必须大于0

#define TO_EPOLL(event)                                                        \
    (((event)&PE_READ) ? EPOLLIN : 0) | (((event)&PE_WRITE) ? EPOLLOUT : 0) |  \
        (((event)&PE_ERROR) ? (EPOLLHUP | EPOLLERR) : 0) |                     \
        (((event)&PE_LT) ? 0 : EPOLLET)

#define TO_POLLER(epoll_event)                                                 \
    (((epoll_event)&EPOLLIN) ? PE_READ : 0) |                                  \
        (((epoll_event)&EPOLLOUT) ? PE_WRITE : 0) |                            \
        (((epoll_event)&EPOLLHUP) ? PE_ERROR : 0) |                            \
        (((epoll_event)&EPOLLERR) ? PE_ERROR : 0)

namespace common_library {

EventPoller::EventPoller()
{
    // 将写设置为非阻塞，防止poller线程退出后，因写满缓存被永久阻塞
    SocketUtils::SetNoBlocked(pipe_.WriteFD());
    SocketUtils::SetNoBlocked(pipe_.ReadFD());

    epoll_fd_ = epoll_create(EPOLL_SIZE);
    if (epoll_fd_ == -1) {
        std::runtime_error(StringPrinter << "epoll create failed. "
                                         << get_uv_errmsg());
    }

    if (AddEvent(pipe_.ReadFD(), PE_READ,
                 [this](int event) { on_pipe_event(); }) == -1) {
        throw std::runtime_error(StringPrinter << "epoll add event failed. "
                                               << get_uv_errmsg());
    }
}

EventPoller::~EventPoller()
{
    Shutdown();

    if (epoll_fd_ != -1) {
        ::close(epoll_fd_);
        epoll_fd_ = -1;
    }

    // 退出前清空管道
    on_pipe_event();
}

EventPoller::Ptr EventPoller::Create()
{
    EventPoller::Ptr ptr(new EventPoller());
    return ptr;
}

Task::Ptr EventPoller::Async(TaskIn&& task)
{
    return async(std::move(task), false);
}

Task::Ptr EventPoller::AsyncFirst(TaskIn&& task)
{
    return async(std::move(task), true);
}

int EventPoller::AddEvent(int fd, int event, PollEventCB&& cb)
{
    TimeTicker();
    if (!cb) {
        return -1;
    }

    struct epoll_event ev = {0};
    ev.events             = TO_EPOLL(event);
    ev.data.fd            = fd;

    int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
    if (ret == 0) {
        event_map_.emplace(fd, std::make_shared<PollEventCB>(cb));
    }

    return ret;
}

int EventPoller::DelEvent(int fd, PollDelCB&& cb)
{
    TimeTicker();

    if (!cb) {
        cb = [](bool success) {};
    }

    bool success = true;
    if (event_map_.count(fd)) {
        success = epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) == 0 &&
                  event_map_.erase(fd);
        cb(success);
    }
    return success ? 0 : -1;
}

int EventPoller::ModifyEvent(int fd, int event)
{
    TimeTicker();

    struct epoll_event ev;
    ev.events  = TO_EPOLL(event);
    ev.data.fd = fd;
    // epoll线程安全
    return epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
}

DelayTask::Ptr EventPoller::DoDelayTask(uint64_t                    delay_ms,
                                        std::function<uint64_t()>&& task)
{
    DelayTask::Ptr ret      = std::make_shared<DelayTask>(std::move(task));
    uint64_t       timeline = delay_ms + get_current_milliseconds();
    AsyncFirst(
        [this, timeline, ret]() { delay_tasks_.emplace(timeline, ret); });
    return ret;
}

void EventPoller::RunLoop()
{
    set_thread_name("poller");
    set_thread_priority(TPRIORITY_HIGHEST);

    exit_flag_ = false;

    uint64_t           delay_ms;
    struct epoll_event events[EPOLL_SIZE];
    while (!exit_flag_) {
        delay_ms = get_min_delay_ms();
        int n =
            epoll_wait(epoll_fd_, events, EPOLL_SIZE, delay_ms ? delay_ms : -1);
        if (n <= 0) {
            // 超时或被中断
            continue;
        }

        for (int i = 0; i < n; ++i) {
            struct epoll_event& event = events[i];
            int                 fd    = event.data.fd;
            std::unordered_map<int, std::shared_ptr<PollEventCB>>::iterator it =
                event_map_.find(fd);
            if (it == event_map_.end()) {
                epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
                continue;
            }

            std::shared_ptr<PollEventCB> cb = it->second;
            try {
                (*cb)(TO_POLLER(event.events));
            }
            catch (std::exception& e) {
                LOG_E << "event poller caught an exception while executing "
                         "event handler. "
                      << e.what();
            }
        }
    }
}

void EventPoller::Shutdown()
{
    async([]() { throw ExitException(); }, true);
}

inline void EventPoller::on_pipe_event()
{
    TimeTicker();

    char buf[1024];

    do {
        // 清空pipe
        if (pipe_.Read(buf, sizeof(buf)) > 0) {
            continue;
        }
    } while (get_uv_error() != EAGAIN);

    List<Task::Ptr> swap_list;
    swap_list.swap(task_list_);
    swap_list.for_each([&](const Task::Ptr& f) {
        try {
            (*f)();
        }
        catch (ExitException& e) {
            exit_flag_ = true;
        }
        catch (std::exception& e) {
            LOG_E << "event poller caught an exception while executing task. "
                  << e.what();
        }
    });
}

uint64_t EventPoller::get_min_delay_ms()
{
    std::multimap<uint64_t, DelayTask::Ptr>::iterator it = delay_tasks_.begin();

    if (it == delay_tasks_.end()) {
        return 0;
    }

    uint64_t now = get_current_milliseconds();
    if (it->first > now) {
        return it->first - now;
    }

    return flush_delay_tasks(now);
}

uint64_t EventPoller::flush_delay_tasks(uint64_t now)
{
    std::multimap<uint64_t, DelayTask::Ptr> delay_tasks_copy;
    delay_tasks_copy.swap(delay_tasks_);

    for (std::multimap<uint64_t, DelayTask::Ptr>::iterator it =
             delay_tasks_copy.begin();
         it != delay_tasks_copy.end() && it->first <= now;
         it = delay_tasks_copy.erase(it)) {
        try {
            // 执行已到期任务，并将循环定时任务再次放入队列
            uint64_t next_delay = (*(it->second))();
            if (next_delay > 0) {
                delay_tasks_copy.emplace(next_delay + now,
                                         std::move(it->second));
            }
        }
        catch (std::exception& e) {
            LOG_E << "event poller caught an exception while executing delay "
                     "task. "
                  << e.what();
        }
    }

    // @warning 这句不需要？因为只有poll线程会操作这个delay_tasks_
    // delay_tasks_copy.insert(delay_tasks_.begin(), delay_tasks_.end());
    delay_tasks_copy.swap(delay_tasks_);

    std::multimap<uint64_t, DelayTask::Ptr>::iterator it = delay_tasks_.begin();
    if (it == delay_tasks_.end()) {
        return 0;
    }

    return it->first - now;
}

Task::Ptr EventPoller::async(TaskIn&& task, bool first)
{
    TimeTicker();

    Task::Ptr ptask = std::make_shared<Task>(std::move(task));
    {
        if (first) {
            task_list_.emplace_front(ptask);
        }
        else {
            task_list_.emplace_back(ptask);
        }
    }

    pipe_.Write("", 1);

    return ptask;
}

}  // namespace common_library