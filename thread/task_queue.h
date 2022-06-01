#ifndef COMMON_LIBRARY_TASK_QUEUE_H
#define COMMON_LIBRARY_TASK_QUEUE_H

#include <thread/semaphore.h>
#include <thread/task.h>
#include <utils/list.h>

namespace common_library {

template <typename T> class TaskQueue final {
  public:
    TaskQueue()  = default;
    ~TaskQueue() = default;

  public:
    template <typename Func> void AddTask(Func&& f)
    {
        {
            std::lock_guard<std::mutex> lock(mux_);
            tasks_.emplace_back(std::forward<Func>(f));
        }
        sem_.Post();
    }

    template <typename Func> void AddTaskFirst(Func&& f)
    {
        {
            std::lock_guard<std::mutex> lock(mux_);
            tasks_.emplace_front(std::forward<Func>(f));
        }
        sem_.Post();
    }

    void PostExit(uint32_t waiting_thread_num)
    {
        sem_.Post(waiting_thread_num);
    }

    bool GetTask(T& f)
    {
        sem_.Wait();

        std::lock_guard<std::mutex> lock(mux_);
        if (tasks_.empty()) {
            return false;
        }

        f = std::move(tasks_.front());
        tasks_.pop_front();

        return true;
    }

    uint64_t Size()
    {
        std::lock_guard<std::mutex> lock(mux_);
        return tasks_.size();
    }

  private:
    List<T>    tasks_;
    Semaphore  sem_;
    std::mutex mux_;
};

}  // namespace common_library

#endif