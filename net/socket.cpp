#include <net/socket.h>
#include <utils/uv_error.h>
#include <thread/worker.h>

namespace common_library {

Socket::Socket(const EventPoller::Ptr& poller, bool enable_mutex)
    : mux_sockfd_(enable_mutex), mux_buffer_waiting_(enable_mutex),
      mux_buffer_sending_(enable_mutex), mux_event_(enable_mutex)
{
}

void Socket::Close()
{
    con_timer_.reset();
    async_connect_cb_.reset();
    LOCK_GUARD(mux_sockfd_);
    sockfd_.reset();
}

void Socket::Connect(const std::string& url,
                     uint16_t           port,
                     const OnErrCB&     connect_cb,
                     float              timeout_sec,
                     const char*        local_ip,
                     uint16_t           local_port)
{
    Close();

    EventPoller::Ptr      poller    = poller_;
    std::weak_ptr<Socket> weak_self = shared_from_this();
    std::shared_ptr<bool> timeouted = std::make_shared<bool>(false);

    std::shared_ptr<std::function<void(int)>> async_connect_cb =
        std::make_shared<std::function<void(int)>>(
            [weak_self, timeouted, connect_cb](int sock) {
                std::shared_ptr<Socket> strong_self = weak_self.lock();
                // 已经超时或者被销毁
                if (!strong_self || *timeouted) {
                    if (sock != -1) {
                        close(sock);
                    }
                    return;
                }

                if (sock == -1) {
                    // 连接服务器失败，dns失败导致
                    connect_cb(SockException(SockErr_dns, get_uv_errmsg()));
                    // 取消超时定时器
                    strong_self->con_timer_.reset();
                    return;
                }

                SockFd::Ptr sockfd =
                    strong_self->make_sockfd(sock, SockNum::Sock_TCP);
                std::weak_ptr<SockFd> weak_sockfd = sockfd;

                strong_self->poller_->AddEvent(
                    sock, PE_WRITE,
                    [weak_self, weak_sockfd, connect_cb, timeouted](int event) {
                        // 将fd放入epoll去监听，如果可写，说明已连接上
                        Socket::Ptr strong_self   = weak_self.lock();
                        SockFd::Ptr strong_sockfd = weak_sockfd.lock();

                        if (!strong_self || !strong_sockfd || *timeouted) {
                            // 已超时或被销毁
                            return;
                        }

                        // strong_self->on_connected(
                    });
            });
        
    
}

SockFd::Ptr Socket::make_sockfd(int sock, SockNum::SockType type)
{
    return std::make_shared<SockFd>(sock, type, poller_);
}

void Socket::on_connected(const SockFd::Ptr& sockfd, const OnErrCB& connect_cb)
{
    con_timer_.reset();

    SockException sock_exception = get_sock_err(sockfd, false);

    do {
        if (!sock_exception) {
            // 连接成功，无错误发生，将fd从epoll中移除
            poller_->DelEvent(sockfd->RawFd());
        }
    } while (0);
}

int Socket::on_read(const SockFd::Ptr& sockfd, bool is_udp)
{
    int             ret = 0, nread = 0, sock = sockfd->RawFd();
    struct sockaddr peeraddr;
    socklen_t       len         = sizeof(peeraddr);
    BufferRaw::Ptr  read_buffer = read_buffer_;
    char*           data        = read_buffer->Data();
    uint32_t        capactiy    = read_buffer->Capacity() - 1;

    while (enable_recv_) {
        do {
            nread = recvfrom(sock, data, capactiy, 0, &peeraddr, &len);
        } while (nread == -1 && get_uv_error() == EINTR);

        if (nread == 0) {
            if (!is_udp) {
                emit_err(SockException(SockErr_eof, "end of file"));
            }
            return ret;
        }

        if (nread == -1) {
            if (get_uv_error() != EAGAIN) {
                on_error(sockfd);
            }
            return ret;
        }

        ret += nread;
        data[nread] = '\0';

        read_buffer->SetSize(nread);

        LOCK_GUARD(mux_event_);
        read_cb_(read_buffer, &peeraddr, len);
    }
}

int on_writeable(const SockFd::Ptr& sockfd)
{
    bool empty_waiting;
    bool empty_sending;

    {
    }
}

void Socket::on_error(const SockFd::Ptr& sockfd)
{
    emit_err(get_sock_err(sockfd));
}

SockException Socket::get_sock_err(const SockFd::Ptr& sockfd, bool try_errno)
{
    int error = 0, len = sizeof(int);

    getsockopt(sockfd->RawFd(), SOL_SOCKET, SO_ERROR, (char*)&error,
               (socklen_t*)&len);
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
        case EAGAIN: return SockException(SockErr_success, "success");
        case ECONNREFUSED:
            return SockException(SockErr_refused, uv_strerror(error), error);
        case ETIMEDOUT:
            return SockException(SockErr_timeout, uv_strerror(error), error);
        default: return SockException(SockErr_other, uv_strerror(error), error);
    }
}

bool Socket::attach_event(const SockFd::Ptr& sockfd, bool is_udp)
{
    std::weak_ptr<Socket> weak_self   = shared_from_this();
    std::weak_ptr<SockFd> weak_sockfd = sockfd;

    enable_recv_ = true;
    if (!read_buffer_) {
        // udp 包最大可以达到65535字节
        read_buffer_ =
            std::make_shared<BufferRaw>(is_udp ? 0xFFFF : 128 * 1024);
    }

    int result =
        poller_->AddEvent(sockfd->RawFd(), PE_READ | PE_WRITE | PE_ERROR,
                          [weak_self, weak_sockfd, is_udp](int event) {
                              Socket::Ptr strong_self   = weak_self.lock();
                              SockFd::Ptr strong_sockfd = weak_sockfd.lock();
                              if (!strong_self || !strong_sockfd) {
                                  return;
                              }

                              if (event & PE_ERROR) {
                                  strong_self->on_error(strong_sockfd);
                              }
                              if (event & PE_READ) {
                                  strong_self->on_read(strong_sockfd, is_udp);
                              }
                              if (event & PE_WRITE) {}
                          });

    return true;
}

bool Socket::emit_err(const SockException& err)
{
    {
        LOCK_GUARD(mux_sockfd_);
        if (!sockfd_) {
            return false;
        }
    }
    // @warning 这里不用加锁？
    Close();

    std::weak_ptr<Socket> weak_self = shared_from_this();

    poller_->Async([weak_self, err]() {
        std::shared_ptr<Socket> strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }

        LOCK_GUARD(strong_self->mux_event_);
        strong_self->err_cb_(err);
    });

    return true;
}

bool Socket::flush_data(const SockFd::Ptr sockfd, bool is_poller_thread)
{
    List<BufferList::Ptr> buffer_sending_tmp;
    {
        LOCK_GUARD(mux_buffer_sending_);
        if (!buffer_sending_.empty()) {
            buffer_sending_tmp.swap(buffer_sending_);
        }
    }

    if (buffer_sending_tmp.empty()) {
        last_flush_ticker_.Reset();
        do {
            {
                LOCK_GUARD(mux_buffer_waiting_);
                if (!buffer_waiting_.empty()) {
                    // buffer_sending_tmp.emplace_back(
                        // std::make_shared<BufferList>(buffer_waiting_));
                    break;
                }
            }
        } while (0);
    }
}

}  // namespace common_library