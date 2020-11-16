#ifndef COMMON_LIBRARY_SOCKET_UTILS_H
#define COMMON_LIBRARY_SOCKET_UTILS_H

#include <stdint.h>

#include <string>

struct sockaddr_storage;

namespace common_library {

class SocketUtils {
  public:
    static int CreateSocket(bool is_ipv6);

    static int Connect(const char* host,
                       uint16_t    port,
                       const char* local_ip   = "0.0.0.0",
                       uint16_t    local_port = 0,
                       bool        async      = false);

    static int
    Bind(int fd, const char* local_ip, uint16_t port, bool is_ipv6 = false);

    static int SetNoBlocked(int fd, bool enable = true);

    static int SetReuseable(int fd, bool enable = true);

    static int SetKeepAlive(int fd, bool enable = true);

    static int SetNoDelay(int fd, bool enable = true);

    static int SetSendBuf(int fd, int size = 256 * 1024);

    static int SetRecvBuf(int fd, int size = 256 * 1024);

    static int SetCloExec(int fd, bool enable = true);

    static int SetCloseWait(int fd, int seconds = 0);

    static int SetSendTimeout(int fd, int seconds = 10);

    static std::string GetIPFromAddr(sockaddr_storage* addr);

    static uint16_t GetPortFromAddr(sockaddr_storage* addr);

    static std::string GetLocalIP(int fd);

    static std::string GetPeerIP(int fd);

    static uint16_t GetLocalPort(int fd);

    static uint16_t GetPeerPort(int fd);

    static int Listen(uint16_t    port,
                      bool        is_ipv6,
                      const char* local_ip = "0.0.0.0",
                      int         backlog  = 1024);
};
}  // namespace common_library

#endif