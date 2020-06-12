#ifndef COMMON_LIBRARY_TIME_TICKER_H
#define COMMON_LIBRARY_TIME_TICKER_H

#include <utils/utils.h>

namespace common_library {

class Ticker final {
  public:
    Ticker()
    {
        create_time_ms_ = begin_time_ms_ = get_current_milliseconds();
    }

    ~Ticker() = default;

  public:
    uint64_t ElapsedTimeMS()
    {
        return get_current_milliseconds() - begin_time_ms_;
    }

    uint64_t CreateTimeMS()
    {
        return get_current_milliseconds() - create_time_ms_;
    }

    void Reset()
    {
        begin_time_ms_ = get_current_milliseconds();
    }

  private:
    uint64_t create_time_ms_;
    uint64_t begin_time_ms_;
};

}  // namespace common_library

#endif