#ifndef COMMON_LIBRARY_LOAD_COUNTER_H
#define COMMON_LIBRARY_LOAD_COUNTER_H

#include <utils/list.h>
#include <utils/utils.h>

#include <mutex>

namespace common_library {

class ThreadLoadCounter {
  public:
    ThreadLoadCounter()          = default;
    virtual ~ThreadLoadCounter() = default;

  public:
    virtual int Load() = 0;
};

class ThreadLoadCounterImpl final : public ThreadLoadCounter {
  public:
    ThreadLoadCounterImpl(uint64_t max_size, uint32_t max_duration_usec)
    {
        last_sleep_time_usec_ = last_wake_time_usec_ =
            get_current_microseconds();
        max_size_          = max_size;
        max_duration_usec_ = max_duration_usec;
    }
    ~ThreadLoadCounterImpl() = default;

  public:
    int Load() override
    {
        std::unique_lock<std::mutex> lock(mux_);

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

  public:
    void Sleep()
    {
        std::unique_lock<std::mutex> lock(mux_);

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
        std::unique_lock<std::mutex> lock(mux_);

        sleep_ = false;

        uint64_t now_usec        = get_current_microseconds();
        uint64_t sleep_time_usec = now_usec - last_sleep_time_usec_;
        last_wake_time_usec_     = now_usec;

        time_lists_.emplace_back(sleep_time_usec, true);
        if (time_lists_.size() > max_size_) {
            time_lists_.pop_front();
        }
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