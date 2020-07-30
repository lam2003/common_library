#include <poller/timer.h>
#include <utils/logger.h>

namespace common_library {

Timer::Timer(float                        second,
             const std::function<bool()>& cb,
             const EventPoller::Ptr&      poller,
             bool                         continue_when_exception)
{
    poller_ = poller;
    if (!poller_) {
        poller_ = EventPollerPool::Instance().GetPoller();
    }

    tag_ = poller_->DoDelayTask(
        second * 1000, [cb, second, continue_when_exception]() {
            try {
                if (cb()) {
                    return (uint64_t)(second * 1000);
                }
                return (uint64_t)0;
            }
            catch (std::exception& e) {
                LOG_E << "timer caught an exception while executing task. "
                      << e.what();

                return continue_when_exception ? (uint64_t)(second * 1000) :
                                                 (uint64_t)0;
            }
        });
}

Timer::~Timer()
{
    DelayTask::Ptr tag = tag_.lock();
    if (tag) {
        tag->Cancel();
    }
}
}  // namespace common_library