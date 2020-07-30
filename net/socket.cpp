#include <net/socket.h>

namespace common_library {

Socket::Socket(const EventPoller::Ptr& poller, bool enable_mutex)
    : mux_sockfd_(enable_mutex)
{
}

}  // namespace common_library