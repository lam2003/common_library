#ifndef COMMON_LIBRARY_EVENT_POLLER_H
#define COMMON_LIBRARY_EVENT_POLLER_H

#include <poller/pipe_wrapper.h>
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

typedef std::function<void(int event)> PollEventCB;

class EventPoller final : public TaskExecutor,
                          public std::enable_shared_from_this<EventPoller> {
  public:
    typedef std::shared_ptr<EventPoller> Ptr;
    ~EventPoller();

  public:
    Task::Ptr Async(TaskIn&& task, bool may_sync = true) override;

    Task::Ptr AsyncFirst(TaskIn&& task, bool may_sync = true) override;

  public:
    int AddEvent(int fd, int event, PollEventCB&& cb);

    bool IsCurrentThread();

  private:
    EventPoller(ThreadPriority priority);

  private:
    class ExitException : public std::exception {
      public:
        ExitException()  = default;
        ~ExitException() = default;
    };

    void on_pipe_event();

  private:
    ThreadPriority                                        priority_;
    std::thread::id                                       loop_tid_;
    PipeWrapper                                           pipe_;
    std::unordered_map<int, std::shared_ptr<PollEventCB>> event_map_;
    List<Task::Ptr>                                       task_list_;
    std::mutex                                            task_mux_;
    bool                                                  exit_flag_;
    int                                                   epoll_fd_ = -1;
};
}  // namespace common_library

#endif