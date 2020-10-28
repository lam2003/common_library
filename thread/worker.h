#ifndef COMMON_LIBRARY_THREAD_POOL_H
#define COMMON_LIBRARY_THREAD_POOL_H

#include <thread/group.h>
#include <thread/task_executor.h>
#include <thread/task_queue.h>
#include <utils/logger.h>
#include <utils/utils.h>

#include <vector>

namespace common_library {

class Worker final : public TaskExecutor {
  public:
    Worker(ThreadPriority priority = TPRIORITY_HIGHEST, bool auto_run = true)
    {
        priority_ = priority;
        if (auto_run) {
            Start();
        }
    }

    ~Worker()
    {
        Shutdown();
        Wait();
    }

  public:
    int Load() override
    {
        return load_counter_->Load();
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
        // 先初始化counters
        load_counter_ =
            std::make_shared<ThreadLoadCounterImpl>(32, 2 * 1000 * 1000);
        thread_group_.CreateThread(std::bind(&Worker::run, this));
    }

    void Shutdown()
    {
        queue_.PostExit(1);
    }

    void Wait()
    {
        thread_group_.JoinAll();
    }

  private:
    void run()
    {
        set_thread_priority(priority_);
        Task::Ptr ptask = nullptr;
        while (true) {
            load_counter_->Sleep();
            if (!queue_.GetTask(ptask)) {
                break;
            }
            load_counter_->WakeUp();
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
    ThreadPriority             priority_;
    ThreadGroup                thread_group_;
    ThreadLoadCounterImpl::Ptr load_counter_;
    TaskQueue<Task::Ptr>       queue_;
};

class WorkerPool final : public std::enable_shared_from_this<WorkerPool>,
                         public TaskExecutorGetter {
  public:
    typedef std::shared_ptr<WorkerPool> Ptr;

    ~WorkerPool() = default;

  public:
    static WorkerPool& Instance();

    static void SetPoolSize(int size = 0);

    Worker::Ptr GetWorker();

  private:
    WorkerPool();

  private:
    static int s_pool_size_;
};

}  // namespace common_library

#endif