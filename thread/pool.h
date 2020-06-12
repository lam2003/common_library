#ifndef COMMON_LIBRARY_THREAD_POOL_H
#define COMMON_LIBRARY_THREAD_POOL_H

#include <thread/group.h>
#include <thread/task_executor.h>
#include <thread/task_queue.h>
#include <utils/logger.h>

#include <sched.h>

#include <vector>

namespace common_library {
class ThreadPool : public TaskExecutor {
  public:
    enum Priority {
        PRIORITY_LOWEST = 0,
        PRIORITY_LOW,
        PRIORITY_NORMAL,
        PRIORITY_HIGH,
        PRIORITY_HIGHEST
    };
    ThreadPool(int      thread_num = 1,
               Priority priority   = PRIORITY_HIGHEST,
               bool     auto_run   = true)
    {
        thread_num_ = thread_num;
        priority_   = priority;
        if (auto_run) {
            Start();
        }
    }

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

    int Load() override
    {
        int total_load = 0;

        for (int i = 0; i < thread_num_; i++) {
            total_load += counters_[i]->Load();
        }

        return total_load / thread_num_;
    }

    void Start()
    {
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

    static bool SetPriority(Priority priority = PRIORITY_NORMAL,
                            std::thread::native_handle_type tid = 0)
    {
        static int min = sched_get_priority_min(SCHED_OTHER);
        if (min == -1) {
            return false;
        }
        static int max = sched_get_priority_max(SCHED_OTHER);
        if (max == -1) {
            return false;
        }

        static int priorities[] = {min, min + (max - min) / 4,
                                   min + (max - min) / 2,
                                   min + (max - min) * 3 / 4, max};

        if (tid == 0) {
            tid = pthread_self();
        }
        struct sched_param params;
        params.sched_priority = priorities[priority];
        return pthread_setschedparam(tid, SCHED_OTHER, &params) == 0;
    }

  private:
    void run(int counter_index)
    {
        SetPriority(priority_);

        Task::Ptr                              ptask = nullptr;
        std::shared_ptr<ThreadLoadCounterImpl> counter =
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
                // TODO: add log
                LOG_E << "thread pool execution task caught exception: "
                      << e.what();
            }
        }
    }

  private:
    int                                                 thread_num_;
    Priority                                            priority_;
    ThreadGroup                                         thread_group_;
    std::vector<std::shared_ptr<ThreadLoadCounterImpl>> counters_;
    TaskQueue<Task::Ptr>                                queue_;
};

}  // namespace common_library

#endif