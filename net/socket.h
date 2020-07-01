#ifndef COMMON_LIBRARY_SOCKET_H
#define COMMON_LIBRARY_SOCKET_H

#include <utils/noncopyable.h>

#include <functional>
#include <memory>
#include <string>

namespace common_library {

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
    // typedef std::function<void(const)>

    Socket();
};

}  // namespace common_library

#endif