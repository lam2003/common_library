#ifndef COMMON_LIBRARY_TASK_EXECUTOR_H
#define COMMON_LIBRARY_TASK_EXECUTOR_H

#include <thread/load_counter.h>
#include <thread/semaphore.h>
#include <thread/task.h>
#include <utils/once_token.h>

namespace common_library {

class TaskExecutor : public ThreadLoadCounter {
  public:
    TaskExecutor(uint64_t max_size          = 32,
                 uint64_t max_duration_usec = 2 * 1000 * 1000)
        : ThreadLoadCounter(max_size, max_duration_usec)
    {
    }
    virtual ~TaskExecutor() = default;

    virtual Task::Ptr Async(TaskIn&& task, bool may_sync = true) = 0;
    virtual Task::Ptr AsyncFirst(TaskIn&& task, bool may_sync = true)
    {
        return Async(std::move(task), may_sync);
    }

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

}  // namespace common_library

#endif