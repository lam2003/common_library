#ifndef COMMON_LIBRARY_TIMER_H
#define COMMON_LIBRARY_TIMER_H

#include <poller/event_poller.h>

namespace common_library {

class Timer {
  public:
    typedef std::shared_ptr<Timer> Ptr;
    Timer(float                        second,
          const std::function<bool()>& cb,
          const EventPoller::Ptr&      poller,
          bool                         continue_when_exception = true);
    ~Timer();

  private:
    std::weak_ptr<DelayTask> tag_;
    EventPoller::Ptr         poller_;
};

}  // namespace common_library

#endif