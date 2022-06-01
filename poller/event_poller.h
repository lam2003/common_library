#ifndef COMMON_LIBRARY_EVENT_POLLER_H
#define COMMON_LIBRARY_EVENT_POLLER_H

#include <poller/pipe_wrapper.h>
#include <thread/task.h>
#include <thread/task_executor.h>
#include <utils/list.h>
#include <utils/utils.h>

#include <unordered_map>

namespace common_library {

typedef enum {
    PE_READ  = 1 << 0,
    PE_WRITE = 1 << 1,
    PE_ERROR = 1 << 2,
    PE_LT    = 1 << 3
} PollEvent;

typedef std::function<void(int event)>     PollEventCB;
typedef std::function<void(bool success)>  PollDelCB;
typedef TaskCancelableImpl<uint64_t(void)> DelayTask;

class EventPoller final : public TaskExecutor,
                          public std::enable_shared_from_this<EventPoller> {
  public:
    typedef std::shared_ptr<EventPoller> Ptr;
    ~EventPoller();

    static EventPoller::Ptr Create();

  public:
    Task::Ptr Async(TaskIn&& task) override;

    Task::Ptr AsyncFirst(TaskIn&& task) override;

  public:
    int AddEvent(int fd, int event, PollEventCB&& cb);

    int DelEvent(int fd, PollDelCB&& cb = nullptr);

    int ModifyEvent(int fd, int event);

    DelayTask::Ptr DoDelayTask(uint64_t                    delay_ms,
                               std::function<uint64_t()>&& task);

    /**
     * 执行事件循环
     */
    void RunLoop();

    /**
     * 结束事件循环
     * 一旦结束，就不能再次恢复循环
     */
    void Shutdown();

    bool IsClose();

  private:
    EventPoller();

  private:
    class ExitException : public std::exception {
      public:
        ExitException()  = default;
        ~ExitException() = default;
    };

    void on_pipe_event();

    uint64_t get_min_delay_ms();

    uint64_t flush_delay_tasks(uint64_t now);

    Task::Ptr async(TaskIn&& task, bool first);

  private:
    PipeWrapper                                           pipe_;
    std::unordered_map<int, std::shared_ptr<PollEventCB>> event_map_;
    List<Task::Ptr>                                       task_list_;
    bool                                                  exit_flag_;
    Semaphore                                             run_started_sem_;
    std::multimap<uint64_t, DelayTask::Ptr>               delay_tasks_;
    int                                                   epoll_fd_ = -1;
};

}  // namespace common_library

#endif