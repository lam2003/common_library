#include <net/socket.h>
#include <net/socket_utils.h>
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

namespace common_library {

inline LogContextCapturer& Socket::socket_log(const LogContextCapturer& logger,
                                              Socket*                   ptr)
{
    LogContextCapturer& l = const_cast<LogContextCapturer&>(logger);

    if (!ptr->sockfd_) {
        return l;
    }

    if (ptr->sockfd_->IsConnected()) {
        l << ptr->GetIdentifier() << " " << ptr->GetLocalIP() << ":"
          << ptr->GetLocalPort() << "-->" << ptr->GetPeerIP() << ":"
          << ptr->GetPeerPort();
    }
    else {
        l << ptr->GetIdentifier();
    }

    return l;
}

Socket::Ptr Socket::Create(const EventPoller::Ptr& poller)
{
    return std::make_shared<Socket>(poller);
}

Socket::Socket(const EventPoller::Ptr& poller)
{
    poller_ = poller;
    SetOnError(nullptr);
    SetOnFlushed(nullptr);
    SetOnRead(nullptr);
}

Socket::~Socket()
{
    Close();
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
            strong_self->sockfd_ = nullptr;
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
                std::make_shared<SocketFD>(fd, SOCK_TCP, strong_self->poller_);
            std::weak_ptr<SocketFD> weak_sockfd = sockfd;
            strong_self->sockfd_                = sockfd;

            int ret = strong_self->poller_->AddEvent(
                fd, PE_WRITE, [connect_cb, weak_self, weak_sockfd](int event) {
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

bool Socket::Listen(uint16_t           port,
                    bool               is_ipv6,
                    const std::string& local_ip,
                    int                backlog)
{
    Close();

    int fd = SocketUtils::Listen(port, is_ipv6, local_ip.c_str(), backlog);
    if (fd == -1) {
        return false;
    }
    return listen(SocketFD::Create(fd, SOCK_TCP, poller_));
}

void Socket::Close()
{
    connect_timer_    = nullptr;
    async_connect_cb_ = nullptr;
    sockfd_           = nullptr;
}

void Socket::SetOnError(ErrorCB&& cb)
{
    if (cb) {
        error_cb_ = std::move(cb);
    }
    else {
        error_cb_ = [this](const SocketException& err) {};
    }
}

void Socket::SetOnFlushed(FlushedCB&& cb)
{
    if (cb) {
        flushed_cb_ = std::move(cb);
    }
    else {
        flushed_cb_ = [this]() { return true; };
    }
}

void Socket::SetOnRead(ReadCB&& cb)
{
    if (cb) {
        read_cb_ = std::move(cb);
    }
    else {
        read_cb_ = [this](const Buffer::Ptr&, sockaddr_storage*, socklen_t) {};
    }
}

void Socket::SetOnAccept(AcceptCB&& cb)
{
    if (cb) {
        accept_cb_ = std::move(cb);
    }
    else {
        accept_cb_ = [this](Socket::Ptr& socket) {};
    }
}

SocketFD::Ptr Socket::SetPeerSocket(int fd)
{
    Close();
    SocketFD::Ptr sockfd = SocketFD::Create(fd, SOCK_TCP, poller_);
    sockfd_              = sockfd;
    return sockfd_;
}

std::string Socket::GetLocalIP() const
{
    if (!sockfd_) {
        return "";
    }
    return SocketUtils::GetLocalIP(sockfd_->RawFD());
}

std::string Socket::GetPeerIP() const
{
    if (!sockfd_) {
        return "";
    }
    return SocketUtils::GetPeerIP(sockfd_->RawFD());
}

uint16_t Socket::GetLocalPort() const
{
    if (!sockfd_) {
        return 0;
    }
    return SocketUtils::GetLocalPort(sockfd_->RawFD());
}

uint16_t Socket::GetPeerPort() const
{
    if (!sockfd_) {
        return 0;
    }
    return SocketUtils::GetPeerPort(sockfd_->RawFD());
}

bool Socket::IsConnected() const
{
    if (!sockfd_) {
        return false;
    }
    return sockfd_->IsConnected();
}

std::string Socket::GetIdentifier() const
{
    std::ostringstream oss;
    oss << this;
    return oss.str();
}

void Socket::stop_writeable_event(const SocketFD::Ptr& sockfd)
{
    int event = 0;
    if (enable_recv_) {
        event |= PE_READ;
    }

    poller_->ModifyEvent(sockfd->RawFD(), event | PE_ERROR);
    sending_ = false;
}

void Socket::start_writeable_event(const SocketFD::Ptr& sockfd)
{
    int event = 0;
    if (enable_recv_) {
        event |= PE_READ;
    }

    poller_->ModifyEvent(sockfd->RawFD(), event | PE_ERROR | PE_WRITE);
    sending_ = true;
}

bool Socket::flush_data(const SocketFD::Ptr& sockfd)
{
    List<BufferList::Ptr> send_buf_sending_tmp;
    {
        if (!send_buf_sending_.empty()) {
            send_buf_sending_tmp.swap(send_buf_sending_);
        }
    }

    if (send_buf_sending_tmp.empty()) {
        send_flush_ticker_.Reset();
        do {
            {
                if (!send_buf_waiting_.empty()) {
                    send_buf_sending_tmp.emplace_back(
                        std::make_shared<BufferList>(send_buf_waiting_));
                    break;
                }
            }

            stop_writeable_event(sockfd);
            on_flushed();
            return true;

        } while (0);
    }

    int  fd     = sockfd->RawFD();
    bool is_udp = sockfd->Type() == SOCK_UDP;

    while (!send_buf_sending_tmp.empty()) {
        BufferList::Ptr& pkt = send_buf_sending_tmp.front();

        int n = pkt->send(fd, socket_flags_, is_udp);
        if (n > 0) {
            if (pkt->empty()) {
                send_buf_sending_tmp.pop_front();
                continue;
            }

            break;
        }

        // n <= 0
        int err = get_uv_error();
        if (err == EAGAIN) {
            break;
        }

        on_error(sockfd);
        return false;
    }

    if (!send_buf_sending_tmp.empty()) {
        send_buf_sending_tmp.swap(send_buf_sending_);
        send_buf_sending_.append(send_buf_sending_tmp);
        return true;
    }

    // sending缓存已经全部发送完毕，说明该socket还可写，尝试继续写
    // 如果是poller线程，我们尝试再次写一次(因为可能其他线程调用了send函数又有新数据了)
    return flush_data(sockfd);
}

bool Socket::attach_event(const SocketFD::Ptr& sockfd, bool is_udp)
{
    if (!read_buf_) {
        read_buf_ = std::make_shared<BufferRaw>(is_udp ? 0xFFFF : 128 * 1024);
    }

    std::weak_ptr<Socket>   weak_self   = shared_from_this();
    std::weak_ptr<SocketFD> weak_sockfd = sockfd;
    int                     ret =
        poller_->AddEvent(sockfd->RawFD(), PE_READ | PE_WRITE | PE_ERROR,
                          [weak_self, weak_sockfd, is_udp](int event) {
                              auto strong_self   = weak_self.lock();
                              auto strong_sockfd = weak_sockfd.lock();

                              if (!strong_self || !strong_sockfd) {
                                  return;
                              }

                              if (event & PE_READ) {
                                  strong_self->on_read(strong_sockfd, is_udp);
                              }
                              // 如果读时错误已经发生了，停止后面的事件处理
                              if (strong_self->sockfd_ && (event & PE_WRITE)) {
                                  strong_self->on_writeable(strong_sockfd);
                              }
                              if (strong_self->sockfd_ && (event & PE_ERROR)) {
                                  strong_self->on_error(strong_sockfd);
                              }
                          });

    return ret != -1;
}

void Socket::on_connected(const SocketFD::Ptr& sockfd, const ErrorCB& cb)
{
    SocketException err = get_socket_error(sockfd);
    if (err) {
        cb(err);
        return;
    }

    sockfd->SetConnected();
    poller_->DelEvent(sockfd->RawFD());
    if (!attach_event(sockfd, false)) {
        cb(SocketException(ERR_OTHER,
                           "attach to poller failed when connected."));
        return;
    }
    cb(err);
}

int Socket::on_read(const SocketFD::Ptr& sockfd, bool is_udp)
{
    int  ret    = 0;
    int  nread  = 0;
    auto buffer = read_buf_;

    auto             data     = buffer->Data();
    int              capacity = buffer->Capacity() - 1;
    sockaddr_storage addr;
    socklen_t        len = sizeof(addr);

    while (enable_recv_) {
        do {
            nread = ::recvfrom(sockfd->RawFD(), data, capacity, 0,
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

        if (read_cb_) {
            read_cb_(buffer, &addr, len);
        }
    }

    return 0;
}

void Socket::on_writeable(const SocketFD::Ptr& sockfd)
{
    bool sending_empty;
    bool waiting_empty;

    sending_empty = send_buf_sending_.empty();
    waiting_empty = send_buf_waiting_.empty();

    if (sending_empty && waiting_empty) {
        stop_writeable_event(sockfd);
    }
    else {
        flush_data(sockfd);
    }
}

bool Socket::on_error(const SocketFD::Ptr& sockfd)
{
    return emit_error(get_socket_error(sockfd));
}

bool Socket::emit_error(const SocketException& err)
{
    if (!sockfd_) {
        return false;
    }
    Close();

    std::weak_ptr<Socket> weak_self = shared_from_this();
    poller_->Async([weak_self, err]() {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }
        if (strong_self->error_cb_) {
            strong_self->error_cb_(err);
        }
    });

    return true;
}

void Socket::on_flushed()
{
    bool flag = false;

    if (flushed_cb_) {
        flag = flushed_cb_();
    }

    if (!flag) {
        SetOnFlushed(nullptr);
    }
}

int Socket::on_accept(const SocketFD::Ptr& sockfd, int event)
{
    int fd;
    while (true) {
        if (event & PE_READ) {
            do {
                fd = accept(sockfd->RawFD(), nullptr, nullptr);
            } while (fd == -1 && get_uv_error() == EINTR);

            if (fd == -1) {
                int err = get_uv_error();
                if (err == EAGAIN) {
                    // 握手完成队列的连接已经处理完了
                    return 0;
                }
                socket_log(LOG_E, this)
                    << " accept failed. " << uv_strerror(err);
                on_error(sockfd);
                return -1;
            }

            SocketUtils::SetNoBlocked(fd, true);
            SocketUtils::SetNoDelay(fd);
            SocketUtils::SetRecvBuf(fd);
            SocketUtils::SetSendBuf(fd);
            SocketUtils::SetCloseWait(fd);
            SocketUtils::SetCloExec(fd);

            Socket::Ptr   peer_socket = Socket::Create(poller_);
            SocketFD::Ptr peer_sockfd = peer_socket->SetPeerSocket(fd);
            peer_sockfd->SetConnected();

            if (accept_cb_) {
                accept_cb_(peer_socket);
            }

            if (!peer_socket->attach_event(peer_sockfd)) {
                peer_socket->emit_error(SocketException(
                    ERR_OTHER, "attach to poller failed while accept"));
            }
        }

        if (event & PE_ERROR) {
            socket_log(LOG_E, this) << " listen failed. " << get_uv_errmsg();
            on_error(sockfd);
            return -1;
        }
    }

    // should never reach here
    return 0;
}

bool Socket::listen(const SocketFD::Ptr& sockfd)
{
    std::weak_ptr<Socket>   weak_self   = shared_from_this();
    std::weak_ptr<SocketFD> weak_sockfd = sockfd;

    int ret =
        poller_->AddEvent(sockfd->RawFD(), PE_READ | PE_ERROR,
                          [weak_self, weak_sockfd](int event) {
                              auto strong_self   = weak_self.lock();
                              auto strong_sockfd = weak_sockfd.lock();

                              if (!strong_self || !strong_sockfd) {
                                  return;
                              }
                              strong_self->on_accept(strong_sockfd, event);
                          });

    if (ret == -1) {
        return false;
    }

    sockfd_ = sockfd;

    return true;
}

int Socket::Send(const char*      buf,
                 int              size,
                 struct sockaddr* addr,
                 socklen_t        len)
{
    if (size <= 0) {
        size = strlen(buf);
        if (!size) {
            return 0;
        }
    }
    BufferRaw::Ptr ptr = std::make_shared<BufferRaw>();
    ptr->Assign(buf, size);
    return send(ptr, addr, len);
}

int Socket::send(const Buffer::Ptr& buf, sockaddr* addr, socklen_t len)
{
    auto size = buf ? buf->Size() : 0;
    if (!size) {
        return 0;
    }

    if (!sockfd_) {
        return -1;
    }

    std::weak_ptr<Socket>   weak_self   = shared_from_this();
    std::weak_ptr<SocketFD> weak_sockfd = sockfd_;
    Buffer::Ptr             tmp_buf     = (sockfd_->Type() == SOCK_UDP ?
                               std::make_shared<BufferSock>(buf, addr, len) :
                               buf);

    poller_->AsyncFirst([weak_self, weak_sockfd, tmp_buf]() {
        auto strong_self   = weak_self.lock();
        auto strong_sockfd = weak_sockfd.lock();
        if (!strong_self || !strong_sockfd) {
            return;
        }

        strong_self->send_buf_waiting_.emplace_back(tmp_buf);

        if (!strong_self->sending_) {
            strong_self->start_writeable_event(strong_sockfd);
        }
    });

    return 0;
}

SocketException Socket::get_socket_error(const SocketFD::Ptr& sockfd,
                                         bool                 try_errno)
{
    int error = 0, len = sizeof(int);
    getsockopt(sockfd->RawFD(), SOL_SOCKET, SO_ERROR, &error,
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
