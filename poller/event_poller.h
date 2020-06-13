#ifndef COMMON_LIBRARY_EVENT_POLLER_H
#define COMMON_LIBRARY_EVENT_POLLER_H

#include <thread/pool.h>
#include <thread/task_executor.h>

namespace common_library {
class EventPoller final : public TaskExecutor,
                          public std::enable_shared_from_this<EventPoller> {
  public:
    typedef std::shared_ptr<EventPoller> Ptr;
    ~EventPoller();

  public:
    Task::Ptr Async(TaskIn&& task, bool may_sync = true) override;

    Task::Ptr AsyncFirst(TaskIn&& task, bool may_sync = true) override;

  private:
    EventPoller(ThreadPool::Priority priority);

  private:
    ThreadPool::Priority priority_;
};
}  // namespace common_library

#endif