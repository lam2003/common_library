#ifndef COMMON_LIBRARY_MUTEX_WRAPPER_H
#define COMMON_LIBRARY_MUTEX_WRAPPER_H

#include <mutex>

namespace common_library {

template <typename Mux = std::recursive_mutex> class MutexWrapper {
  public:
    MutexWrapper(bool enable)
    {
        enable_ = enable;
    }

    ~MutexWrapper() {}

    inline void Lock()
    {
        if (enable_) {
            mux_.lock();
        }
    }

    inline void Unlock()
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