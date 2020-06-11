#ifndef COMMON_LIBRARY_TASK_EXECUTOR_H
#define COMMON_LIBRARY_TASK_EXECUTOR_H

#include <thread/semaphore.h>
#include <utils/list.h>
#include <utils/noncopyable.h>
#include <utils/once_token.h>
#include <utils/utils.h>

#include <memory>

namespace common_library {

class TaskCancelable : public noncopyable {
  public:
    TaskCancelable()          = default;
    virtual ~TaskCancelable() = default;
    virtual void Cancel()     = 0;
};

template <typename T> class TaskCancelableImpl;

template <typename Ret, typename... Args>
class TaskCancelableImpl<Ret(Args...)> : public TaskCancelable {
  public:
    typedef std::shared_ptr<TaskCancelableImpl> ptr;
    typedef std::function<Ret(Args...)>         function_type;

    ~TaskCancelableImpl() = default;

    template <typename Func> TaskCancelableImpl(Func&& f)
    {
        strong_task_ = std::make_shared<function_type>(std::forward<Func>(f));
        weak_task_   = strong_task_;
    }

    void Cancel() override
    {
        strong_task_ = nullptr;
    }

    // return cancelable
    operator bool()
    {
        return strong_task_ && *strong_task_;
    }

    Ret operator()(Args... args) const
    {
        std::shared_ptr<function_type> strong_task = weak_task_.lock();
        if (strong_task && *strong_task) {
            return (*strong_task)(std::forward<Args>(args)...);
        }
        return default_return<Ret>();
    }

  protected:
    template <typename T>
    static typename std::enable_if<std::is_void<T>::value, void>::type
    default_return()
    {
    }

    template <typename T>
    static typename std::enable_if<std::is_integral<T>::value, T>::type
    default_return()
    {
        return 0;
    }

    template <typename T>
    static typename std::enable_if<std::is_pointer<T>::value, T>::type
    default_return()
    {
        return nullptr;
    }

  private:
    std::shared_ptr<function_type> strong_task_;
    std::weak_ptr<function_type>   weak_task_;
};

typedef std::function<void()>      TaskIn;
typedef TaskCancelableImpl<void()> Task;

class TaskExecutorInterface {
  public:
    TaskExecutorInterface()          = default;
    virtual ~TaskExecutorInterface() = default;

    virtual Task::ptr Async(TaskIn&& task, bool may_sync = true) = 0;
    virtual Task::ptr AsyncFirst(TaskIn&& task, bool may_sync = true)
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

class ThreadLoadCounter {
  public:
    ThreadLoadCounter(uint64_t max_size, uint32_t max_duration_usec)
    {
        last_sleep_time_usec_ = last_wake_time_usec_ =
            get_current_microseconds();
        max_size_          = max_size;
        max_duration_usec_ = max_duration_usec;
    }
    ~ThreadLoadCounter() {}

    void Sleep()
    {
        std::unique_lock<std::mutex> mux_;

        sleep_ = true;

        uint64_t now_usec      = get_current_microseconds();
        uint64_t run_time_usec = now_usec - last_wake_time_usec_;
        last_sleep_time_usec_  = now_usec;

        time_lists_.emplace_back(run_time_usec, false);

        if (time_lists_.size() > max_size_) {
            time_lists_.pop_front();
        }
    }

    void WakeUp()
    {
        std::unique_lock<std::mutex> mux_;

        sleep_ = false;

        uint64_t now_usec        = get_current_microseconds();
        uint64_t sleep_time_usec = now_usec - last_sleep_time_usec_;
        last_wake_time_usec_     = now_usec;

        time_lists_.emplace_back(sleep_time_usec, true);
        if (time_lists_.size() > max_size_) {
            time_lists_.pop_front();
        }
    }

    int Load()
    {
        std::unique_lock<std::mutex> mux_;

        uint64_t run_time_usec   = 0;
        uint64_t sleep_time_usec = 0;

        time_lists_.for_each([&](const TimeRecord& item) {
            if (item.sleep_) {
                sleep_time_usec += item.ts_usec_;
            }
            else {
                run_time_usec += item.ts_usec_;
            }
        });

        uint64_t now_usec = get_current_microseconds();
        if (sleep_) {
            sleep_time_usec += (now_usec - last_sleep_time_usec_);
        }
        else {
            run_time_usec += (now_usec - last_wake_time_usec_);
        }

        uint64_t total_time_usec = sleep_time_usec + run_time_usec;

        // 拟合统计窗口
        while (!time_lists_.empty() && (total_time_usec > max_duration_usec_ ||
                                        time_lists_.size() > max_size_)) {
            const TimeRecord& item = time_lists_.front();
            if (item.sleep_) {
                sleep_time_usec -= item.ts_usec_;
            }
            else {
                run_time_usec -= item.ts_usec_;
            }

            total_time_usec -= item.ts_usec_;
            time_lists_.pop_front();
        }

        // 避免n/0问题
        if (total_time_usec == 0) {
            return 0;
        }

        return run_time_usec * 100 / total_time_usec;
    }

  private:
    struct TimeRecord
    {
        TimeRecord(uint64_t ts_usec, bool sleep)
        {
            ts_usec_ = ts_usec;
            sleep_   = sleep;
        }
        uint64_t ts_usec_;
        bool     sleep_;
    };

  private:
    uint64_t         last_sleep_time_usec_;
    uint64_t         last_wake_time_usec_;
    uint64_t         max_size_;
    uint64_t         max_duration_usec_;
    bool             sleep_ = false;
    List<TimeRecord> time_lists_;
    std::mutex       mux_;
};

}  // namespace common_library

#endif