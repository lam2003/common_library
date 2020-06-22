#include <net/socket_utils.h>
#include <poller/event_poller.h>
#include <utils/time_ticker.h>
#include <utils/utils.h>
#include <utils/uv_error.h>

#include <sys/epoll.h>

#define EPOLL_SIZE 1024  // 必须大于0

#define TO_EPOLL(event)                                                        \
    (((event)&PE_READ) ? EPOLLIN : 0) | (((event)&PE_WRITE) ? EPOLLOUT : 0) |  \
        (((event)&PE_ERROR) ? (EPOLLHUP | EPOLLERR) : 0) |                     \
        (((event)&PE_LT) ? 0 : EPOLLET)

#define TO_POLLER(epoll_event)                                                 \
    (((epoll_event)&EPOLLIN) ? PE_READ : 0) |                                  \
        (epoll_event)(((epoll_event)&EPOLLOUT) ? PE_WRITE : 0) |               \
        (epoll_event)(((epoll_event)&EPOLLHUP) ? PE_ERROR : 0) |               \
        (epoll_event)(((epoll_event)&EPOLLERR) ? PE_ERROR : 0)

namespace common_library {

EventPoller::EventPoller(ThreadPriority priority)
{
    priority_ = priority;
    set_socket_non_blocked(pipe_.WriteFD());
    set_socket_non_blocked(pipe_.ReadFD());

    epoll_fd_ = epoll_create(EPOLL_SIZE);
    if (epoll_fd_ == -1) {
        std::runtime_error(StringPrinter << "epoll create failed. "
                                         << get_uv_errmsg());
    }

    loop_tid_ = std::this_thread::get_id();

    if (AddEvent(pipe_.ReadFD(), PE_READ,
                 [this](int event) { on_pipe_event(); }) == -1) {
        throw std::runtime_error(StringPrinter << "epoll add event failed. "
                                               << get_uv_errmsg());
    }
}

EventPoller::~EventPoller() {}

Task::Ptr EventPoller::Async(TaskIn&& task, bool may_sync)
{
    return nullptr;
}

Task::Ptr EventPoller::AsyncFirst(TaskIn&& task, bool may_sync)
{
    return nullptr;
}

int EventPoller::AddEvent(int fd, int event, PollEventCB&& cb)
{
    TimeTicker();
    if (!cb) {
        return -1;
    }

    if (IsCurrentThread()) {
        struct epoll_event ev = {0};
        ev.events             = TO_EPOLL(event);
        ev.data.fd            = fd;

        int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
        if (ret == 0) {
            event_map_.emplace(fd, std::make_shared<PollEventCB>(cb));
        }

        return ret;
    }

    Async([this, fd, event, cb]() {
        AddEvent(fd, event, std::move(const_cast<PollEventCB&>(cb)));
    });

    return 0;
}

bool EventPoller::IsCurrentThread()
{
    return std::this_thread::get_id() == loop_tid_;
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

    {
        std::lock_guard<std::mutex> lock(task_mux_);
        swap_list.swap(task_list_);
    }

    swap_list.for_each([this](Task::Ptr& f) {
        try {
            (*f)();
        }
        catch (ExitException& e) {
            exit_flag_ = true;
        }
        catch (std::exception& e) {
            LOG_E << "event poller caught an exception: " << e.what();
        }
    });
}

Task::Ptr EventPoller::async(TaskIn&& task, bool may_sync, bool first)
{
    TimeTicker();
    if (may_sync && IsCurrentThread()) {
        task();
        return nullptr;
    }

    std::shared_ptr<Task> ptask = std::make_shared<Task>(std::move(task));
    {
        std::lock_guard<std::mutex> lock(task_mux_);
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