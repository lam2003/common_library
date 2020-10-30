#include <net/socket.h>
#include <net/socket_utils.h>
#include <thread/worker.h>
#include <utils/logger.h>
#include <utils/uv_error.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace common_library {

Socket::Socket(const EventPoller::Ptr& poller, bool enable_mutex)
    : sockfd_mux_(enable_mutex)
{
    poller_ = poller;
}

#define CLOSE_SOCKET(fd)                                                       \
    {                                                                          \
        if (fd != -1) {                                                        \
            ::close(fd);                                                       \
        }                                                                      \
    }

int Socket::Connect(const std::string& host,
                    uint16_t           port,
                    const ErrorCB&     cb,
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
            // 为何不需要用户回调？
            // 外部主动将socket析构，本不希望收到回调，这里还是打一条日志吧
            LOG_W << "socket instance has been destroyed. just reture";
            return;
        }

        strong_self->connect_timer_    = nullptr;
        strong_self->async_connect_cb_ = nullptr;

        if (err) {
            // 错误发生，将fd从epoll中移除
            LOCK_GUARD(strong_self->sockfd_mux_);
            strong_self->sockfd_ = nullptr;
        }

        // 给用户回调connect结果
        cb(err);
    };

    auto async_connect_cb = std::make_shared<std::function<void(int)>>(
        [connect_cb, weak_self](int fd) {
            if (fd == -1) {
                connect_cb(SocketException(ERR_DNS, get_uv_errmsg()));
                return;
            }

            auto strong_self = weak_self.lock();
            if (!strong_self) {
                CLOSE_SOCKET(fd);
                // 为何不需要用户回调？
                // 外部主动将socket析构，本不希望收到回调，这里还是打一条日志吧
                LOG_W << "socket instance has been destroyed. just close fd "
                         "and reture";
                return;
            }

            auto sockfd =
                std::make_shared<SocketFd>(fd, SOCK_TCP, strong_self->poller_);
            std::weak_ptr<SocketFd> weak_sockfd = sockfd;

            int res = strong_self->poller_->AddEvent(
                fd, PE_WRITE, [connect_cb, weak_sockfd, weak_self](int event) {
                    auto strong_self   = weak_self.lock();
                    auto strong_sockfd = weak_sockfd.lock();

                    if (strong_self && strong_sockfd) {
                        // socket可写，说明已经连接成功
                        strong_self->on_connected(strong_sockfd, connect_cb);
                    }
                });

            if (res != 0) {
                connect_cb(SocketException(
                    ERR_OTHER,
                    "add event to poller failed when beginning to connect"));
                return;
            }

            LOCK_GUARD(strong_self->sockfd_mux_);
            strong_self->sockfd_ = sockfd;
        });

    async_connect_cb_ = async_connect_cb;
    connect_timer_    = std::make_shared<Timer>(
        timeout_sec,
        [weak_self, connect_cb]() {
            connect_cb(SocketException(ERR_TIMEOUT, uv_strerror(ETIMEDOUT)));
            return false;
        },
        poller_);

    std::weak_ptr<std::function<void(int)>> weak_async_connect_cb =
        async_connect_cb;

    WorkerPool::Instance().GetWorker()->Async(
        [host, port, local_ip, local_port, weak_async_connect_cb] {
            int fd = SocketUtils::Connect(host.c_str(), port, local_ip.c_str(),
                                          local_port, true);
            
            
        });
    return 0;
}

void Socket::Close()
{
    connect_timer_ = nullptr;
}

void Socket::on_connected(const SocketFd::Ptr& sockfd, const ErrorCB& cb)
{
    SocketException err = get_socket_error(sockfd);
    if (err) {
        cb(err);
        return;
    }

    poller_->DelEvent(sockfd->RawFd());
    if (attach_event(sockfd, false) != 0) {
        cb(SocketException(ERR_OTHER,
                           "attach to poller failed when connected."));
        return;
    }
    cb(err);
}

int Socket::attach_event(const SocketFd::Ptr& sockfd, bool is_udp)
{
    return 0;
}

SocketException Socket::get_socket_error(const SocketFd::Ptr& sockfd,
                                         bool                 try_errno)
{
    int error = 0, len = sizeof(int);
    getsockopt(sockfd->RawFd(), SOL_SOCKET, SO_ERROR, &error,
               reinterpret_cast<socklen_t*>(&len));
    if (error == 0) {
        if (try_errno) {
            error = get_uv_error();
        }
    }
    else {
        error = uv_translate_posix_error(error);
    }

    switch (error) {
        case 0:
        case EAGAIN: return SocketException(ERR_SUCCESS, "OK");
        case ETIMEDOUT: return SocketException(ERR_TIMEOUT, "TIMEOUT");
        case ECONNREFUSED: return SocketException(ERR_REFUESD, "REFUESD");
        default: return SocketException(ERR_OTHER, "OTHER");
    }
}

}  // namespace common_library
