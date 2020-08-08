#ifndef COMMON_LIBRARY_SOCKET_H
#define COMMON_LIBRARY_SOCKET_H

#include <net/buffer.h>
#include <poller/event_poller.h>
#include <poller/timer.h>
#include <utils/list.h>
#include <utils/mutex_wrapper.h>
#include <utils/noncopyable.h>

#include <sys/socket.h>
#include <unistd.h>

#include <exception>
#include <functional>
#include <memory>
#include <string>

namespace common_library {

typedef enum {
    SockErr_success = 0,
    SockErr_eof,
    SockErr_timeout,
    SockErr_refused,
    SockErr_dns,
    SockErr_shutdown,
    SockErr_other = 0xFF
} SockErrCode;

class SockException final : public std::exception {
  public:
    SockException(SockErrCode        err_code    = SockErr_success,
                  const std::string& err_msg     = "",
                  int                custom_code = 0)
    {
        err_msg_     = err_msg;
        err_code_    = err_code;
        custom_code_ = custom_code;
    }

  public:
    virtual const char* what() const noexcept override
    {
        return err_msg_.c_str();
    }

  public:
    void Reset(SockErrCode err_code, const std::string& err_msg)
    {
        err_msg_  = err_msg;
        err_code_ = err_code;
    }

    SockErrCode GetErrCode() const
    {
        return err_code_;
    }

    operator bool() const
    {
        return err_code_ != SockErr_success;
    }

    int GetCustomCode() const
    {
        return custom_code_;
    }

    void SetCustomCode(int custom_code)
    {
        custom_code_ = custom_code;
    }

  private:
    std::string err_msg_;
    SockErrCode err_code_;
    int         custom_code_ = 0;
};

class SockNum {
  public:
    typedef enum { Sock_TCP = 0, Sock_UDP } SockType;
    typedef std::shared_ptr<SockNum> Ptr;
    SockNum(int fd, SockType type)
    {
        fd_   = fd;
        type_ = type;
    }

    ~SockNum()
    {
        // shutdown与close的主要区别：close是引用记数，
        // 多进程时当引用记数为0时才进行四次挥手
        // shutdown是强制进行四次挥手，其他进程再次读写时会触发SIGPIPE信号
        ::shutdown(fd_, SHUT_RDWR);
        ::close(fd_);
    }

  public:
    int RawFd() const
    {
        return fd_;
    }

    SockType Type() const
    {
        return type_;
    }

  private:
    SockType type_;
    int      fd_;
};

class SockFd : public noncopyable {
  public:
    typedef std::shared_ptr<SockFd> Ptr;
    SockFd(int num, SockNum::SockType type, const EventPoller::Ptr& poller)
    {
        num_    = std::make_shared<SockNum>(num, type);
        poller_ = poller;
    }

    SockFd(const SockFd& that, const EventPoller::Ptr& poller)
    {
        num_    = that.num_;
        poller_ = poller;
        if (poller_ == that.poller_) {
            throw std::invalid_argument("copy a sockfd with same poller!");
        }
    }

    ~SockFd()
    {
        // 防止异步删除event_poller事件时fd被率先关闭
        std::shared_ptr<SockNum> num = num_;
        poller_->DelEvent(num_->RawFd(), [num](bool) {});
    }

  public:
    int RawFd() const
    {
        return num_->RawFd();
    }

    SockNum::SockType Type() const
    {
        return num_->Type();
    }

  private:
    SockNum::Ptr     num_;
    EventPoller::Ptr poller_;
};

class SocketInfo {
  public:
    SocketInfo()          = default;
    virtual ~SocketInfo() = default;

  public:
    virtual std::string GetLocalIP()   = 0;
    virtual uint16_t    GetLocalPort() = 0;
    virtual std::string GetPeerIP()    = 0;
    virtual std::string GetIdentifier() const
    {
        return "";
    }
    virtual uint64_t GetPeerPort() = 0;
};

class Socket final : public noncopyable,
                     public std::enable_shared_from_this<Socket>,
                     public SocketInfo {
  public:
    typedef std::shared_ptr<Socket> Ptr;
    // 接收数据回调
    typedef std::function<
        void(const Buffer::Ptr& buf, struct sockaddr* addr, int addr_len)>
        OnReadCB;
    // 错误回调
    typedef std::function<void(const SockException& err)> OnErrCB;
    // TCP监听回调
    typedef std::function<void(Socket::Ptr& sock)> OnAcceptCB;
    // Socket缓存发送完毕回调，通过这个回调可以以最大带宽发送数据
    // 例如http文件下载服务器，返回false则移除该Socket
    typedef std::function<bool()> OnFlush;
    // 在接收到连接请求前，拦截Socket默认生成方式
    typedef std::function<Socket::Ptr(const EventPoller::Ptr& poller)>
        OnBeforeAcceptCB;

    Socket(const EventPoller::Ptr& poller = nullptr, bool enable_mutex = true);

    ~Socket();

  public:
    void Close();

    void Connect(const std::string& url,
                 uint16_t           port,
                 const OnErrCB&     connect_cb,
                 float              timeout_sec = 5,
                 const char*        local_ip    = "0.0.0.0",
                 uint16_t           local_port  = 0);

    bool Listen(uint16_t    port,
                const char* local_ip   = "0.0.0.0",
                uint16_t    local_port = 0);

    bool BindUdpSock(const uint16_t port, const char* local_ip = "0.0.0.0");

  private:
    SockFd::Ptr make_sockfd(int sock, SockNum::SockType type);

    void on_connected(const SockFd::Ptr& sockfd, const OnErrCB& connect_cb);

    int on_read(const SockFd::Ptr& sockfd, bool is_udp);

    int on_writeable(const SockFd::Ptr& sockfd);

    void on_error(const SockFd::Ptr& sockfd);

    SockException get_sock_err(const SockFd::Ptr& sockfd,
                               bool               try_errno = true);

    bool attach_event(const SockFd::Ptr& sockfd, bool is_udp);

    bool emit_err(const SockException& err);

    bool flush_data(const SockFd::Ptr sockfd, bool is_poller_thread);

  private:
    EventPoller::Ptr                   poller_;
    Timer::Ptr                         con_timer_;
    SockFd::Ptr                        sockfd_;
    MutexWrapper<std::recursive_mutex> mux_sockfd_;
    //////////////////////
    std::shared_ptr<std::function<void(int)>> async_connect_cb_;
    //////////////////////
    MutexWrapper<std::recursive_mutex> mux_buffer_waiting_;
    List<BufferList::Ptr>              buffer_waiting_;
    MutexWrapper<std::recursive_mutex> mux_buffer_sending_;
    List<BufferList::Ptr>              buffer_sending_;
    //////////////////////
    std::atomic<bool> enable_recv_;
    //////////////////////
    MutexWrapper<std::recursive_mutex> mux_event_;
    OnErrCB                            err_cb_;
    OnReadCB                           read_cb_;
    //////////////////////
    BufferRaw::Ptr read_buffer_;
    //////////////////////
    Ticker last_flush_ticker_;
};

}  // namespace common_library

#endif