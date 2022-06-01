#ifndef COMMON_LIBRARY_TIME_TICKER_H
#define COMMON_LIBRARY_TIME_TICKER_H

#include <utils/logger.h>
#include <utils/utils.h>

namespace common_library {

class Ticker final {
  public:
    Ticker(uint64_t             min_ms = 0,
           LogContextCapturer&& ctx    = LogContextCapturer(Logger::Instance(),
                                                         LWARN,
                                                         __FILE__,
                                                         "",
                                                         __LINE__),
           bool                 print_log = false)
        : ctx_(ctx)
    {
        if (!print_log) {
            ctx_.Clear();
        }
        create_time_ms_ = begin_time_ms_ = get_current_milliseconds();
        min_ms_                          = min_ms;
    }

    ~Ticker()
    {
        uint64_t tm = CreateTimeMS();
        if (tm > min_ms_) {
            ctx_ << "take time: " << tm << std::endl;
        }
        else {
            ctx_.Clear();
        }
    }

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
    uint64_t           create_time_ms_;
    uint64_t           begin_time_ms_;
    LogContextCapturer ctx_;
    uint64_t           min_ms_;
};

#define TimeTicker() Ticker __ticker(5, LOG_W, true)
#define TimeTicker1(tm) Ticker __ticker1(tm, LOG_W, true)
#define TimeTicker2(tm, log) Ticker __ticker2(tm, log, true)
}  // namespace common_library

#endif