#ifndef COMMON_LIBRARY_TASK_EXECUTOR_H
#define COMMON_LIBRARY_TASK_EXECUTOR_H

#include <thread/semaphore.h>
#include <thread/task.h>
#include <utils/once_token.h>
#include <utils/time_ticker.h>

#include <atomic>
#include <vector>

namespace common_library {

class TaskExecutor {
  public:
    typedef std::shared_ptr<TaskExecutor> Ptr;

    TaskExecutor()          = default;
    virtual ~TaskExecutor() = default;

  public:
    virtual Task::Ptr Async(TaskIn&& task) = 0;

    virtual Task::Ptr AsyncFirst(TaskIn&& task) = 0;
};

}  // namespace common_library

#endif