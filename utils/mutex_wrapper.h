#ifndef COMMON_LIBRARY_MUTEX_WRAPPER_H
#define COMMON_LIBRARY_MUTEX_WRAPPER_H

#include <mutex>

#define LOCK_GUARD(mux) std::lock_guard<decltype(mux)> lock(mux)

namespace common_library {

template <typename Mux = std::recursive_mutex> class MutexWrapper {
  public:
    MutexWrapper(bool enable)
    {
        enable_ = enable;
    }

    ~MutexWrapper() {}

  public:
    inline void lock()
    {
        if (enable_) {
            mux_.lock();
        }
    }

    inline void unlock()
    {
        if (enable_) {
            mux_.unlock();
        }
    }

  private:
    bool enable_;
    Mux  mux_;
};

}  // namespace common_library

#endif