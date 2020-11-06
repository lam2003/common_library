#ifndef COMMON_LIBRARY_SOCKET_H
#define COMMON_LIBRARY_SOCKET_H

#include <exception>
#include <memory>
#include <string>

#include <poller/event_poller.h>
#include <poller/timer.h>
#include <utils/mutex_wrapper.h>

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

class SocketFd final {
  public:
    typedef std::shared_ptr<SocketFd> Ptr;
    SocketFd(int fd, SockType type, const EventPoller::Ptr& poller)
    {
        fd_     = fd;
        type_   = type;
        poller_ = poller;
    }

    ~SocketFd()
    {
        poller_->DelEvent(fd_);

        ::shutdown(fd_, SHUT_RDWR);
        ::close(fd_);
    }

    int RawFd() const
    {
        return fd_;
    }

    SockType Type() const
    {
        return type_;
    }

  private:
    int              fd_;
    SockType         type_;
    EventPoller::Ptr poller_;
};

class Socket final : public std::enable_shared_from_this<Socket> {
  public:
    typedef std::shared_ptr<Socket>                     Ptr;
    typedef std::function<void(const SocketException&)> ErrorCB;

    Socket(const EventPoller::Ptr& poller, bool enable_mutex);

    int Connect(const std::string& host,
                uint16_t           port,
                const ErrorCB&     cb,
                float              timeout_sec = 5,
                const std::string& local_ip    = "0.0.0.0",
                uint16_t           local_port  = 0);

    void Close();

  private:
    void on_connected(const SocketFd::Ptr& sockfd, const ErrorCB& cb);

    int attach_event(const SocketFd::Ptr& sockfd, bool is_udp = false);

    static SocketException get_socket_error(const SocketFd::Ptr& sockfd,
                                            bool try_errno = true);

  private:
    EventPoller::Ptr poller_;
    // 用于connect超时检测
    std::shared_ptr<Timer>                    connect_timer_;
    std::shared_ptr<std::function<void(int)>> async_connect_cb_;

    MutexWrapper<std::mutex> sockfd_mux_;
    SocketFd::Ptr            sockfd_;
};

}  // namespace common_library

#endif