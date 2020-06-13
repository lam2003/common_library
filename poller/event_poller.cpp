#include <poller/event_poller.h>

namespace common_library {

EventPoller::EventPoller(ThreadPool::Priority priority)
{
    priority_ = priority;
}

EventPoller::~EventPoller() {}

Task::Ptr EventPoller::Async(TaskIn&& task, bool may_sync)
{
    return nullptr;
}

Task::Ptr EventPoller::AsyncFirst(TaskIn&& task, bool may_sync)
{
    return nullptr;
}
}  // namespace common_library