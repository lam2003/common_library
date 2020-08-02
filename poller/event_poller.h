#ifndef COMMON_LIBRARY_EVENT_POLLER_H
#define COMMON_LIBRARY_EVENT_POLLER_H

#include <poller/pipe_wrapper.h>
#include <thread/task.h>
#include <thread/task_executor.h>
#include <utils/list.h>
#include <utils/utils.h>

#include <mutex>
#include <thread>
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

class EventPollerPool;

class EventPoller final : public TaskExecutor,
                          public std::enable_shared_from_this<EventPoller> {
  public:
    friend class EventPollerPool;
    typedef std::shared_ptr<EventPoller> Ptr;
    ~EventPoller();

  public:
    Task::Ptr Async(TaskIn&& task, bool may_sync = true) override;

    Task::Ptr AsyncFirst(TaskIn&& task, bool may_sync = true) override;

    int Load() override;

  public:
    int AddEvent(int fd, int event, PollEventCB&& cb);

    int DelEvent(int fd, PollDelCB&& cb = nullptr);

    bool IsCurrentThread();

    DelayTask::Ptr DoDelayTask(uint64_t                    delay_ms,
                               std::function<uint64_t()>&& task);

    /**
     * 执行事件循环
     * @param blocked 是否在当前线程执行事件循环
     * @param register_current_poller 是否注册到全局map
     */
    void RunLoop(bool blocked, bool register_current_poller);

    /**
     * 结束事件循环
     * 一旦结束，就不能再次恢复循环
     */
    void Shutdown();

    /**
     * 等待poller退出
     */
    void Wait();

    static EventPoller::Ptr GetCurrentPoller();

  private:
    EventPoller(ThreadPriority priority = TPRIORITY_HIGHEST);

  private:
    class ExitException : public std::exception {
      public:
        ExitException()  = default;
        ~ExitException() = default;
    };

    void on_pipe_event();

    uint64_t get_min_delay_ms();

    uint64_t flush_delay_tasks(uint64_t now);

    Task::Ptr async(TaskIn&& task, bool may_sync, bool first);

  private:
    ThreadPriority                                        priority_;
    std::thread::id                                       loop_tid_;
    PipeWrapper                                           pipe_;
    std::unordered_map<int, std::shared_ptr<PollEventCB>> event_map_;
    List<Task::Ptr>                                       task_list_;
    std::mutex                                            task_mux_;
    std::mutex                                            running_mux_;
    bool                                                  exit_flag_;
    Semaphore                                             run_started_sem_;
    std::multimap<uint64_t, DelayTask::Ptr>               delay_tasks_;
    ThreadLoadCounterImpl                                 counter_;
    std::thread* loop_thread_ = nullptr;
    int          epoll_fd_    = -1;
};

class EventPollerPool final
    : public std::enable_shared_from_this<EventPollerPool>,
      public TaskExecutorGetter {
  public:
    typedef std::shared_ptr<EventPollerPool> Ptr;

    ~EventPollerPool() = default;

  public:
    static EventPollerPool& Instance();

    static void SetPoolSize(int size = 0);

    EventPoller::Ptr GetFirstPoller();

    EventPoller::Ptr GetPoller();

    void SetPreferCurrentThread(bool flag);

  private:
    EventPollerPool();

  private:
    bool       prefer_current_thread_ = true;
    static int s_pool_size_;
};
}  // namespace common_library

#endif