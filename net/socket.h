#ifndef COMMON_LIBRARY_SOCKET_H
#define COMMON_LIBRARY_SOCKET_H

#include <net/buffer.h>
#include <poller/event_poller.h>
#include <utils/noncopyable.h>

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

    
};

}  // namespace common_library

#endif