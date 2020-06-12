#ifndef COMMON_LIBRARY_THREAD_EXECUTOR_H
#define COMMON_LIBRARY_THREAD_EXECUTOR_H

#include <thread/group.h>
#include <thread/load_counter.h>
#include <thread/semaphore.h>
#include <thread/task.h>
#include <utils/once_token.h>

namespace common_library {

class TaskExecutor : public ThreadLoadCounter {
  public:
    typedef std::shared_ptr<TaskExecutor> Ptr;

    TaskExecutor()          = default;
    virtual ~TaskExecutor() = default;

  public:
    virtual Task::Ptr Async(TaskIn&& task, bool may_sync = true) = 0;

    virtual Task::Ptr AsyncFirst(TaskIn&& task, bool may_sync = true) = 0;

  public:
    void Sync(TaskIn&& task)
    {
        Semaphore             sem;
        std::shared_ptr<Task> ptask = Async([&]() {
            OnceToken(nullptr, [&]() {
                // 利用RAII原理避免发生异常时这行代码未被执行
                sem.Post();
            });
            task();
        });

        if (ptask && *ptask) {
            sem.Wait();
        }
    }

    void SyncFirst(TaskIn&& task, bool may_sync = true)
    {
        Semaphore             sem;
        std::shared_ptr<Task> ptask = AsyncFirst([&]() {
            OnceToken(nullptr, [&]() {
                // 利用RAII原理避免发生异常时这行代码未被执行
                sem.Post();
            });
            task();
        });

        if (ptask && *ptask) {
            sem.Wait();
        }
    }
};

class TaskExecutorGetter {
  public:
    TaskExecutorGetter()  = default;
    ~TaskExecutorGetter() = default;
};

}  // namespace common_library

#endif