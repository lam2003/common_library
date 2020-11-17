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
    virtual Task::Ptr Async(TaskIn&& task, bool may_sync = true) = 0;

    virtual Task::Ptr AsyncFirst(TaskIn&& task, bool may_sync = true) = 0;

  public:
    void Sync(TaskIn&& task)
    {
        Semaphore sem;
        Task::Ptr ptask = Async([&]() {
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
        Semaphore sem;
        Task::Ptr ptask = AsyncFirst([&]() {
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
    TaskExecutorGetter()          = default;
    virtual ~TaskExecutorGetter() = default;

  public:
    TaskExecutor::Ptr GetExecutor()
    {
        TaskExecutor::Ptr executor = executors_[pos_];
        pos_++;
        pos_ %= executors_.size();
        return executor;
    }

  protected:
    template <typename Func> void create_executor(Func&& f, int num)
    {
        for (int i = 0; i < num; i++) {
            executors_.emplace_back(f());
        }
    }

  protected:
    std::vector<TaskExecutor::Ptr> executors_;
    int                            pos_ = 0;
};

}  // namespace common_library

#endif