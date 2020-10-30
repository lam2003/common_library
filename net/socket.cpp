#include <net/socket.h>

namespace common_library {

Socket::Socket(const EventPoller::Ptr& poller, bool enable_mutex)
    : sockfd_mux_(enable_mutex)
{
    poller_ = poller;
}

int Socket::Connect(const std::string& host,
                    uint16_t           port,
                    ErrorCB            cb,
                    float              timeout_sec,
                    const std::string& local_ip,
                    uint16_t           local_port)
{
    std::weak_ptr<Socket> weak_self = shared_from_this();

    // connect_cb
    // 连接最终结果回调，无论如何都会执行
    auto connect_cb = [cb, weak_self](const SocketException& err) {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }

        strong_self->connect_timer_    = nullptr;
        strong_self->async_connect_cb_ = nullptr;

        if (err) {
            LOCK_GUARD(strong_self->sockfd_mux_);
            strong_self->sockfd_ = nullptr;
        }
        else {
        }
    };

    auto async_connect_cb =
        std::make_shared<std::function<void(int)>>([](int fd) {});

    return 0;
}

void Socket::Close()
{
    connect_timer_ = nullptr;
}

}  // namespace common_library
