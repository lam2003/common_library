#ifndef COMMON_LIBRARY_THREAD_POOL_H
#define COMMON_LIBRARY_THREAD_POOL_H

#include <thread/group.h>
#include <thread/task_executor.h>
#include <thread/task_queue.h>
#include <utils/logger.h>
#include <utils/utils.h>

#include <vector>

namespace common_library {

class ThreadPool final : public TaskExecutor {
  public:
    ThreadPool(int            thread_num = 1,
               ThreadPriority priority   = TPRIORITY_HIGHEST,
               bool           auto_run   = true)
    {
        thread_num_ = thread_num;
        priority_   = priority;
        if (auto_run) {
            Start();
        }
    }

    ~ThreadPool()
    {
        Shutdown();
        Wait();
    }

  public:
    int Load() override
    {
        int total_load = 0;

        for (int i = 0; i < thread_num_; i++) {
            total_load += counters_[i]->Load();
        }

        if (thread_num_ == 0) {
            return 0;
        }

        return total_load / thread_num_;
    }

  public:
    Task::Ptr Async(TaskIn&& task, bool may_sync = true) override
    {
        if (may_sync && thread_group_.IsSelfThreadIn()) {
            task();
            return nullptr;
        }

        Task::Ptr ptask = std::make_shared<Task>(std::move(task));
        queue_.AddTask(ptask);
        return ptask;
    }

    Task::Ptr AsyncFirst(TaskIn&& task, bool may_sync = true) override
    {
        if (may_sync && thread_group_.IsSelfThreadIn()) {
            task();
            return nullptr;
        }

        Task::Ptr ptask = std::make_shared<Task>(std::move(task));
        queue_.AddTaskFirst(ptask);
        return ptask;
    }

    void Start()
    {
        if (thread_num_ <= 0)
            return;

        int total = thread_num_ - thread_group_.Size();
        counters_.reserve(total);

        // 先初始化counters
        for (int i = 0; i < total; i++) {
            counters_[i] =
                std::make_shared<ThreadLoadCounterImpl>(32, 2 * 1000 * 1000);
        }

        for (int i = 0; i < total; i++) {
            std::thread* pthread = thread_group_.CreateThread(
                std::bind(&ThreadPool::run, this, i));
        }
    }

    void Shutdown()
    {
        queue_.PostExit(thread_num_);
    }

    void Wait()
    {
        thread_group_.JoinAll();
    }

  private:
    void run(int counter_index)
    {
        set_thread_priority(priority_);

        Task::Ptr                              ptask = nullptr;
        std::shared_ptr<ThreadLoadCounterImpl> &counter =
            counters_[counter_index];

        while (true) {
            counter->Sleep();
            if (!queue_.GetTask(ptask)) {
                break;
            }
            counter->WakeUp();
            try {
                (*ptask)();
                ptask = nullptr;
            }
            catch (std::exception& e) {
                LOG_E << "thread pool caught an exception. " << e.what();
            }
        }
    }

  private:
    int                                                 thread_num_;
    ThreadPriority                                      priority_;
    ThreadGroup                                         thread_group_;
    std::vector<std::shared_ptr<ThreadLoadCounterImpl>> counters_;
    TaskQueue<Task::Ptr>                                queue_;
};

}  // namespace common_library

#endif