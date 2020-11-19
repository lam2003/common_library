#ifndef COMMON_LIBRARY_SOCKET_H
#define COMMON_LIBRARY_SOCKET_H

#include <exception>
#include <memory>
#include <string>

#include <net/buffer.h>
#include <poller/event_poller.h>
#include <poller/timer.h>
#include <utils/mutex_wrapper.h>
#include <utils/noncopyable.h>

#include <sys/socket.h>
#include <unistd.h>

namespace common_library {

typedef enum {
    ERR_SUCCESS = 0,
    ERR_EOF,
    ERR_TIMEOUT,
    ERR_REFUESD,
    ERR_UNREACHABLE,
    ERR_SHUTDOWN,
    ERR_OTHER = 0xFF
} SockErrCode;

typedef enum { SOCK_UDP, SOCK_TCP } SockType;

class SocketException final : public std::exception {
  public:
    SocketException(SockErrCode code = ERR_SUCCESS, const std::string msg = "")
    {
        code_ = code;
        msg_  = msg;
    }

    ~SocketException() = default;

    void Reset(SockErrCode code, const std::string& msg)
    {
        code_ = code;
        msg_  = msg;
    }

    const char* what() const noexcept override
    {
        return msg_.c_str();
    }

    SockErrCode GetErrCode() const
    {
        return code_;
    }

    operator bool() const
    {
        return code_ != ERR_SUCCESS;
    }

  private:
    SockErrCode code_;
    std::string msg_;
};

class SocketFD final {
  public:
    typedef std::shared_ptr<SocketFD> Ptr;

    static SocketFD::Ptr
    Create(int fd, SockType type, const EventPoller::Ptr& poller)
    {
        return std::make_shared<SocketFD>(fd, type, poller);
    }

    SocketFD(int fd, SockType type, const EventPoller::Ptr& poller)
    {
        fd_        = fd;
        type_      = type;
        poller_    = poller;
        connected_ = false;
    }

    ~SocketFD()
    {
        int  fd        = fd_;
        bool connected = connected_;

        // 必须在epoll_ctl之后close fd
        poller_->DelEvent(fd_, [fd, connected](bool success) {
            if (!success) {
                LOG_E << "delete event from poller failed. fd=" << fd;
            }
            if (connected) {
                // 建立连接成功后才调用shutdown
                ::shutdown(fd, SHUT_RDWR);
            }
            ::close(fd);
        });
    }

    int RawFD() const
    {
        return fd_;
    }

    SockType Type() const
    {
        return type_;
    }

    void SetConnected()
    {
        connected_ = true;
    }

    bool IsConnected()
    {
        return connected_;
    }

  private:
    int              fd_;
    SockType         type_;
    EventPoller::Ptr poller_;
    bool             connected_;
};

class SocketInfo {
  public:
    SocketInfo()          = default;
    virtual ~SocketInfo() = default;

  public:
    virtual std::string GetLocalIP() const   = 0;
    virtual std::string GetPeerIP() const    = 0;
    virtual uint16_t    GetLocalPort() const = 0;
    virtual uint16_t    GetPeerPort() const  = 0;
    virtual bool        IsConnected() const  = 0;
    virtual std::string GetIdentifier() const
    {
        return "";
    }
};

class Socket final : public std::enable_shared_from_this<Socket>,
                     public noncopyable,
                     public SocketInfo {
  public:
    typedef std::shared_ptr<Socket>                     Ptr;
    typedef std::function<void(const SocketException&)> ErrorCB;
    typedef std::function<bool()>                       FlushedCB;
    typedef std::function<
        void(const Buffer::Ptr&, sockaddr_storage*, socklen_t)>
                                                     ReadCB;
    typedef std::function<void(Socket::Ptr& socket)> AcceptCB;

    static Socket::Ptr Create(const EventPoller::Ptr& poller, bool enable_mute);

    Socket(const EventPoller::Ptr& poller, bool enable_mutex);

    ~Socket();

    int Connect(const std::string& host,
                uint16_t           port,
                const ErrorCB&     cb,
                float              timeout_sec = 5,
                const std::string& local_ip    = "0.0.0.0",
                uint16_t           local_port  = 0);

    bool Listen(uint16_t           port,
                bool               is_ipv6,
                const std::string& local_ip = "0.0.0.0",
                int                backlog  = 1024);

    void Close();

    int Send(const char*      buf,
             int              size,
             struct sockaddr* addr = nullptr,
             socklen_t        len  = 0);

    void          SetOnError(ErrorCB&& cb);
    void          SetOnFlushed(FlushedCB&& cb);
    void          SetOnRead(ReadCB&& cb);
    void          SetOnAccept(AcceptCB&& cb);
    SocketFD::Ptr SetPeerSocket(int fd);

  public:
    // implement socket info interface
    std::string GetLocalIP() const override;
    std::string GetPeerIP() const override;
    uint16_t    GetLocalPort() const override;
    uint16_t    GetPeerPort() const override;
    bool        IsConnected() const override;
    std::string GetIdentifier() const override;

  private:
    bool attach_event(const SocketFD::Ptr& sockfd, bool is_udp = false);
    void stop_writeable_event(const SocketFD::Ptr& sockfd);
    void start_writeable_event(const SocketFD::Ptr& sockfd);
    bool flush_data(const SocketFD::Ptr& sockfd, bool is_poller_thread);

    void on_connected(const SocketFD::Ptr& sockfd, const ErrorCB& cb);
    int  on_read(const SocketFD::Ptr& sockfd, bool is_udp);
    void on_writeable(const SocketFD::Ptr& sockfd);
    bool on_error(const SocketFD::Ptr& sockfd);
    bool emit_error(const SocketException& err);
    void on_flushed();
    int  on_accept(const SocketFD::Ptr& sockfd, int event);
    bool listen(const SocketFD::Ptr& sockfd);
    int  send(const Buffer::Ptr& buf, sockaddr* addr, socklen_t len);

    static SocketException get_socket_error(const SocketFD::Ptr& sockfd,
                                            bool try_errno = true);
    static inline LogContextCapturer&
    socket_log(const LogContextCapturer& logger, Socket* ptr);

  private:
    EventPoller::Ptr poller_;
    // 用于connect超时检测
    std::shared_ptr<Timer>                    connect_timer_;
    std::shared_ptr<std::function<void(int)>> async_connect_cb_;

    mutable MutexWrapper<std::recursive_mutex> sockfd_mux_;
    SocketFD::Ptr                              sockfd_;

    BufferRaw::Ptr read_buf_ = nullptr;

    Ticker                send_flush_ticker_;
    List<BufferList::Ptr> send_buf_sending_;
    List<Buffer::Ptr>     send_buf_waiting_;

    bool              sending_ = true;
    std::atomic<bool> enable_recv_{true};

    mutable MutexWrapper<std::mutex> cb_mux_;
    ErrorCB                          error_cb_;
    FlushedCB                        flushed_cb_;
    ReadCB                           read_cb_;
    AcceptCB                         accept_cb_;

    int socket_flags_;
};

}  // namespace common_library

#endif