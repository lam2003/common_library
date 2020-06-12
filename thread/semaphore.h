#ifndef COMMON_LIBRARY_THREAD_SEMAPHORE_H
#define COMMON_LIBRARY_THREAD_SEMAPHORE_H

#include <condition_variable>
#include <mutex>

#include <stdint.h>

namespace common_library {

class Semaphore {
  public:
    Semaphore()
    {
        count_ = 0;
    }
    ~Semaphore() = default;

    void Post(uint32_t n = 1)
    {
        std::unique_lock<std::mutex> lock(mux_);
        count_ += n;
        if (n > 1) {
            cond_.notify_all();
        }
        else {
            cond_.notify_one();
        }
    }

    void Wait()
    {
        std::unique_lock<std::mutex> lock(mux_);
        // 防止虚假唤醒
        while (count_ == 0) {
            cond_.wait(lock);
        }
        --count_;
    }

  private:
    uint32_t                    count_;
    std::mutex                  mux_;
    std::condition_variable_any cond_;
};

}  // namespace common_library

#endif