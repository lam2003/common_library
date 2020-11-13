#ifndef COMMON_LIBRARY_SOCKET_UTILS_H
#define COMMON_LIBRARY_SOCKET_UTILS_H

#include <stdint.h>

#include <string>

struct sockaddr_storage;

namespace common_library {

class SocketUtils {
  public:
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

    static std::string Addr2String(sockaddr_storage* addr);
};
}  // namespace common_library

#endif