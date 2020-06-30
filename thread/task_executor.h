#ifndef COMMON_LIBRARY_TASK_EXECUTOR_H
#define COMMON_LIBRARY_TASK_EXECUTOR_H

#include <thread/load_counter.h>
#include <thread/semaphore.h>
#include <thread/task.h>
#include <utils/once_token.h>
#include <utils/time_ticker.h>

#include <atomic>
#include <vector>

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
        size_t pos = pos_;
        if (pos >= executors_.size()) {
            pos = 0;
        }

        TaskExecutor::Ptr min_load_executor = executors_[pos];
        int               min_load          = min_load_executor->Load();

        for (size_t i = 0; i < executors_.size(); i++, pos++) {
            if (pos >= executors_.size()) {
                pos = 0;
            }

            TaskExecutor::Ptr executor = executors_[i];
            int               load     = executor->Load();

            if (load < min_load) {
                min_load_executor = executor;
                min_load          = load;
            }

            if (min_load == 0) {
                break;
            }
        }

        pos_ = pos;

        return min_load_executor;
    }

    std::vector<int> GetExcutorsLoad()
    {
        std::vector<int> vec(executors_.size());
        int              i = 0;

        for (TaskExecutor::Ptr& it : executors_) {
            vec[i++] = it->Load();
        }

        return vec;
    }

    void GetExecutorsDelay(const std::function<void(std::vector<int>&)>& f)
    {
        std::shared_ptr<std::atomic_int> completed =
            std::make_shared<std::atomic_int>(0);
        int                               total_count = executors_.size();
        std::shared_ptr<std::vector<int>> vec =
            std::make_shared<std::vector<int>>(total_count);
        int index = 0;

        for (TaskExecutor::Ptr& it : executors_) {
            std::shared_ptr<Ticker> ticker = std::make_shared<Ticker>();
            it->Async(
                [ticker, completed, total_count, vec, index, f]() {
                    (*vec)[index] = ticker->ElapsedTimeMS();
                    if (++(*completed) == total_count) {
                        f(*vec);
                    }
                },
                false);
            ++index;
        }
    }

    template <typename Func> void for_each(Func&& f)
    {
        for (TaskExecutor::Ptr& it : executors_) {
            f(it);
        }
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