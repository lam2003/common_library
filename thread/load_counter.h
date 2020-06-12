#ifndef COMMON_LIBRARY_LOAD_COUNTER_H
#define COMMON_LIBRARY_LOAD_COUNTER_H

#include <utils/list.h>
#include <utils/utils.h>

#include <mutex>

namespace common_library {

class ThreadLoadCounter {
  public:
    ThreadLoadCounter()  = default;
    ~ThreadLoadCounter() = default;

  public:
    virtual int Load() = 0;
};

class ThreadLoadCounterImpl final : public ThreadLoadCounter {
  public:
    ThreadLoadCounterImpl(uint64_t max_size, uint32_t max_duration_us)
    {
        last_sleep_time_us_ = last_wake_time_us_ = get_current_microseconds();
        max_size_                                = max_size;
        max_duration_us_                         = max_duration_us;
    }
    ~ThreadLoadCounterImpl() = default;

  public:
    int Load() override
    {
        std::lock_guard<std::mutex> lock(mux_);

        uint64_t run_time_us   = 0;
        uint64_t sleep_time_us = 0;

        time_lists_.for_each([&](const TimeRecord& item) {
            if (item.sleep_) {
                sleep_time_us += item.ts_us_;
            }
            else {
                run_time_us += item.ts_us_;
            }
        });

        uint64_t now_us = get_current_microseconds();
        if (sleep_) {
            sleep_time_us += (now_us - last_sleep_time_us_);
        }
        else {
            run_time_us += (now_us - last_wake_time_us_);
        }

        uint64_t total_time_us = sleep_time_us + run_time_us;

        // 拟合统计窗口
        while (!time_lists_.empty() && (total_time_us > max_duration_us_ ||
                                        time_lists_.size() > max_size_)) {
            const TimeRecord& item = time_lists_.front();
            if (item.sleep_) {
                sleep_time_us -= item.ts_us_;
            }
            else {
                run_time_us -= item.ts_us_;
            }

            total_time_us -= item.ts_us_;
            time_lists_.pop_front();
        }

        // 避免n/0问题
        if (total_time_us == 0) {
            return 0;
        }

        return run_time_us * 100 / total_time_us;
    }

  public:
    void Sleep()
    {
        std::lock_guard<std::mutex> lock(mux_);

        sleep_ = true;

        uint64_t now_us      = get_current_microseconds();
        uint64_t run_time_us = now_us - last_wake_time_us_;
        last_sleep_time_us_  = now_us;

        time_lists_.emplace_back(run_time_us, false);

        if (time_lists_.size() > max_size_) {
            time_lists_.pop_front();
        }
    }

    void WakeUp()
    {
        std::lock_guard<std::mutex> lock(mux_);

        sleep_ = false;

        uint64_t now_us        = get_current_microseconds();
        uint64_t sleep_time_us = now_us - last_sleep_time_us_;
        last_wake_time_us_     = now_us;

        time_lists_.emplace_back(sleep_time_us, true);
        if (time_lists_.size() > max_size_) {
            time_lists_.pop_front();
        }
    }

  private:
    struct TimeRecord
    {
        TimeRecord(uint64_t ts_us, bool sleep)
        {
            ts_us_ = ts_us;
            sleep_ = sleep;
        }
        uint64_t ts_us_;
        bool     sleep_;
    };

  private:
    uint64_t         last_sleep_time_us_;
    uint64_t         last_wake_time_us_;
    uint64_t         max_size_;
    uint64_t         max_duration_us_;
    bool             sleep_ = false;
    List<TimeRecord> time_lists_;
    std::mutex       mux_;
};
}  // namespace common_library

#endif