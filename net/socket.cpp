#include <net/socket.h>
#include <net/socket_utils.h>
#include <thread/worker.h>
#include <utils/logger.h>
#include <utils/uv_error.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define CLOSE_SOCKET(fd)                                                       \
    {                                                                          \
        if (fd != -1) {                                                        \
            ::close(fd);                                                       \
        }                                                                      \
    }

#define SOCKET_LOG(logger, ptr, err)                                           \
    logger << ptr->GetIdentifier() << " " << ptr->GetLocalIP() << ":"          \
           << ptr->GetLocalPort() << "-->" << ptr->GetPeerIP() << ":"          \
           << ptr->GetPeerPort() << ", " << err.what();

namespace common_library {

Socket::Socket(const EventPoller::Ptr& poller, bool enable_mutex)
    : sockfd_mux_(enable_mutex), send_buf_sending_mux_(enable_mutex),
      send_buf_waiting_mux_(enable_mutex), cb_mux_(enable_mutex)
{
    poller_ = poller;
    if (!poller_) {
        poller_ = EventPollerPool::Instance().GetPoller();
    }

    SetOnError(nullptr);
    SetOnFlushed(nullptr);
    SetOnRead(nullptr);
}

Socket::~Socket()
{
    Close();
    LOG_I << this;
}

int Socket::Connect(const std::string& host,
                    uint16_t           port,
                    const ErrorCB&     cb,
                    float              timeout_sec,
                    const std::string& local_ip_or_intf,
                    uint16_t           local_port)
{
    Close();
    std::weak_ptr<Socket> weak_self = shared_from_this();

    auto connect_cb = [cb, weak_self](const SocketException& err) {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }

        strong_self->connect_timer_    = nullptr;
        strong_self->async_connect_cb_ = nullptr;

        if (err) {
            // 错误发生，将fd从epoll中移除
            LOCK_GUARD(strong_self->sockfd_mux_);
            strong_self->sockfd_ = nullptr;
            SOCKET_LOG(LOG_E, strong_self, err)
        }
        else {
            SOCKET_LOG(LOG_I, strong_self, err);
        }

        // 给用户回调connect结果
        cb(err);
    };

    auto async_connect_cb = std::make_shared<std::function<void(int)>>(
        [connect_cb, weak_self](int fd) {
            auto strong_self = weak_self.lock();
            if (!strong_self) {
                CLOSE_SOCKET(fd);
                // 为何不需要用户回调？
                // 外部主动将socket析构，本不希望收到回调，这里还是打一条日志吧
                LOG_D << "socket instance has been destroyed. just close fd "
                         "and reture ####2";
                return;
            }

            if (fd == -1) {
                connect_cb(
                    SocketException(ERR_UNREACHABLE, uv_strerror(ENETUNREACH)));
                return;
            }

            auto sockfd =
                std::make_shared<SocketFd>(fd, SOCK_TCP, strong_self->poller_);
            std::weak_ptr<SocketFd> weak_sockfd = sockfd;
            {
                LOCK_GUARD(strong_self->sockfd_mux_);
                strong_self->sockfd_ = sockfd;
            }

            int ret = strong_self->poller_->AddEvent(
                fd, PE_WRITE, [connect_cb, weak_sockfd, weak_self](int event) {
                    auto strong_self   = weak_self.lock();
                    auto strong_sockfd = weak_sockfd.lock();

                    if (strong_self && strong_sockfd) {
                        // socket可写，说明已经连接成功
                        strong_self->on_connected(strong_sockfd, connect_cb);
                    }
                });

            if (ret != 0) {
                connect_cb(SocketException(
                    ERR_OTHER,
                    "add event to poller failed when beginning to connect"));
                return;
            }
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

    auto poller = poller_;

    // 支持单线程，不使用WorkerPool
    poller_->Async([host, port, local_ip_or_intf, local_port,
                    weak_async_connect_cb, poller] {
        int fd = SocketUtils::Connect(
            host.c_str(), port, local_ip_or_intf.c_str(), local_port, true);
        poller->Async([weak_async_connect_cb, fd]() {
            auto strong_async_connect_cb = weak_async_connect_cb.lock();
            if (strong_async_connect_cb) {
                (*strong_async_connect_cb)(fd);
            }
            else {
                LOG_D << "socket instance has been destroyed. just close "
                         "fd and return ####1";
                CLOSE_SOCKET(fd);
            }
        });
    });

    return 0;
}

void Socket::Close()
{
    connect_timer_    = nullptr;
    async_connect_cb_ = nullptr;

    LOCK_GUARD(sockfd_mux_);
    sockfd_ = nullptr;
}

void Socket::SetOnError(ErrorCB&& cb)
{
    LOCK_GUARD(cb_mux_);
    if (cb) {
        error_cb_ = std::move(cb);
    }
    else {
        error_cb_ = [this](const SocketException& err) {};
    }
}

void Socket::SetOnFlushed(FlushedCB&& cb)
{
    LOCK_GUARD(cb_mux_);
    if (cb) {
        flushed_cb_ = std::move(cb);
    }
    else {
        flushed_cb_ = [this]() { return true; };
    }
}

void Socket::SetOnRead(ReadCB&& cb)
{
    LOCK_GUARD(cb_mux_);
    if (cb) {
        read_cb_ = std::move(cb);
    }
    else {
        read_cb_ = [this](const Buffer::Ptr&, sockaddr_storage*, socklen_t) {};
    }
}

std::string Socket::GetLocalIP() const
{
    LOCK_GUARD(sockfd_mux_);
    if (!sockfd_) {
        return "";
    }
    return SocketUtils::GetLocalIP(sockfd_->RawFd());
}

std::string Socket::GetPeerIP() const
{
    LOCK_GUARD(sockfd_mux_);
    if (!sockfd_) {
        return "";
    }
    return SocketUtils::GetPeerIP(sockfd_->RawFd());
}

uint16_t Socket::GetLocalPort() const
{
    LOCK_GUARD(sockfd_mux_);
    if (!sockfd_) {
        return 0;
    }
    return SocketUtils::GetLocalPort(sockfd_->RawFd());
}

uint16_t Socket::GetPeerPort() const
{
    LOCK_GUARD(sockfd_mux_);
    if (!sockfd_) {
        return 0;
    }
    return SocketUtils::GetPeerPort(sockfd_->RawFd());
}

std::string Socket::GetIdentifier() const
{
    static std::string class_name = "Socket:";
    return class_name + std::to_string(reinterpret_cast<uint64_t>(this));
}

void Socket::stop_writeable_event(const SocketFd::Ptr& sockfd)
{
    int event = 0;
    if (enable_recv_.load(std::memory_order_acquire)) {
        event |= PE_READ;
    }

    poller_->ModifyEvent(sockfd->RawFd(), event | PE_ERROR);
}

void Socket::start_writeable_event(const SocketFd::Ptr& sockfd)
{
    int event = 0;
    if (enable_recv_.load(std::memory_order_acquire)) {
        event |= PE_READ;
    }

    poller_->ModifyEvent(sockfd->RawFd(), event | PE_ERROR | PE_WRITE);
}

bool Socket::flush_data(const SocketFd::Ptr& sockfd, bool is_poller_thread)
{
    List<BufferList::Ptr> send_buf_sending_tmp;
    {
        LOCK_GUARD(send_buf_sending_mux_);
        if (!send_buf_sending_.empty()) {
            send_buf_sending_tmp.swap(send_buf_sending_);
        }
    }

    if (send_buf_sending_tmp.empty()) {
        send_flush_ticker_.Reset();
        do {
            {
                LOCK_GUARD(send_buf_waiting_mux_);
                if (!send_buf_waiting_.empty()) {
                    send_buf_sending_tmp.emplace_back(
                        std::make_shared<BufferList>(send_buf_waiting_));
                    break;
                }
            }

            // sending/waiting都为空
            if (is_poller_thread) {
                stop_writeable_event(sockfd);
                on_flushed();
            }
            return true;
        } while (0);
    }

    int  fd     = sockfd->RawFd();
    bool is_udp = sockfd->Type() == SOCK_UDP;

    while (!send_buf_sending_tmp.empty()) {
        BufferList::Ptr& pkt = send_buf_sending_tmp.front();

        int n = pkt->send(fd, socket_flags_, is_udp);
        if (n > 0) {
            if (pkt->empty()) {
                send_buf_sending_tmp.pop_front();
                continue;
            }

            if (!is_poller_thread) {
                start_writeable_event(sockfd);
            }
            break;
        }

        // n <= 0
        int err = get_uv_error();
        if (err == EAGAIN) {
            if (!is_poller_thread) {
                start_writeable_event(sockfd);
            }
            break;
        }

        on_error(sockfd);
        return false;
    }

    if (!send_buf_sending_tmp.empty()) {
        LOCK_GUARD(send_buf_sending_mux_);
        send_buf_sending_tmp.swap(send_buf_sending_);
        send_buf_sending_.append(send_buf_sending_tmp);
        return true;
    }

    // sending缓存已经全部发送完毕，说明该socket还可写，尝试继续写
    // 如果是poller线程，我们尝试再次写一次(因为可能其他线程调用了send函数又有新数据了)
    return is_poller_thread ? flush_data(sockfd, true) : true;
}

bool Socket::attach_event(const SocketFd::Ptr& sockfd, bool is_udp)
{
    if (!read_buf_) {
        read_buf_ = std::make_shared<BufferRaw>(is_udp ? 0xFFFF : 128 * 1024);
    }

    std::weak_ptr<Socket>   weak_self   = shared_from_this();
    std::weak_ptr<SocketFd> weak_sockfd = sockfd;
    int                     ret =
        poller_->AddEvent(sockfd->RawFd(), PE_READ | PE_WRITE | PE_ERROR,
                          [weak_self, weak_sockfd, is_udp](int event) {
                              auto strong_self   = weak_self.lock();
                              auto strong_sockfd = weak_sockfd.lock();

                              if (!strong_self || !strong_sockfd) {
                                  return;
                              }

                              if (event & PE_READ) {
                                  strong_self->on_read(strong_sockfd, is_udp);
                              }
                              if (event & PE_WRITE) {
                                  strong_self->on_writeable(strong_sockfd);
                              }
                              if (event & PE_ERROR) {
                                  strong_self->on_error(strong_sockfd);
                              }
                          });

    return ret != -1;
}

void Socket::on_connected(const SocketFd::Ptr& sockfd, const ErrorCB& cb)
{
    SocketException err = get_socket_error(sockfd);
    if (err) {
        cb(err);
        return;
    }

    sockfd->SetConnected();
    poller_->DelEvent(sockfd->RawFd());
    if (!attach_event(sockfd, false)) {
        cb(SocketException(ERR_OTHER,
                           "attach to poller failed when connected."));
        return;
    }
    cb(err);
}

int Socket::on_read(const SocketFd::Ptr& sockfd, bool is_udp)
{
    int  ret    = 0;
    int  nread  = 0;
    auto buffer = read_buf_;

    auto             data     = buffer->Data();
    int              capacity = buffer->Capacity() - 1;
    sockaddr_storage addr;
    socklen_t        len = sizeof(addr);

    while (enable_recv_.load(std::memory_order_acquire)) {
        do {
            nread = ::recvfrom(sockfd->RawFd(), data, capacity, 0,
                               reinterpret_cast<sockaddr*>(&addr), &len);
        } while (nread == -1 && get_uv_error() == EINTR);

        if (nread == 0) {
            if (!is_udp) {
                emit_error(SocketException(ERR_EOF, "read eof"));
            }
            return ret;
        }

        if (nread == -1) {
            // 如果errno==EAGAIN，socket读缓存被取完，正常返回
            if (get_uv_error() != EAGAIN) {
                on_error(sockfd);
            }
            return ret;
        }

        ret += nread;
        data[nread] = '\0';
        buffer->SetSize(nread);

        LOCK_GUARD(cb_mux_);
        if (read_cb_) {
            read_cb_(buffer, &addr, len);
        }
    }

    return 0;
}

void Socket::on_writeable(const SocketFd::Ptr& sockfd)
{
    bool sending_empty;
    {
        LOCK_GUARD(send_buf_sending_mux_);
        sending_empty = send_buf_sending_.empty();
    }
    bool waiting_empty;
    {
        LOCK_GUARD(send_buf_waiting_mux_);
        waiting_empty = send_buf_waiting_.empty();
    }

    if (sending_empty && waiting_empty) {
        stop_writeable_event(sockfd);
    }
    else {
        flush_data(sockfd, true);
    }
}

bool Socket::on_error(const SocketFd::Ptr& sockfd)
{
    return emit_error(get_socket_error(sockfd));
}

bool Socket::emit_error(const SocketException& err)
{
    {
        LOCK_GUARD(sockfd_mux_);
        SOCKET_LOG(LOG_E, this, err);
        if (!sockfd_) {
            return false;
        }
        Close();
    }

    std::weak_ptr<Socket> weak_self = shared_from_this();
    poller_->Async([weak_self, err]() {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }
        LOCK_GUARD(strong_self->cb_mux_);
        if (strong_self->error_cb_) {
            strong_self->error_cb_(err);
        }
    });

    return true;
}

void Socket::on_flushed()
{
    bool flag = false;

    {
        LOCK_GUARD(cb_mux_);
        if (flushed_cb_) {
            flag = flushed_cb_();
        }
    }
    if (!flag) {
        SetOnFlushed(nullptr);
    }
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
        default: return SocketException(ERR_OTHER, uv_strerror(error));
    }
}

}  // namespace common_library
