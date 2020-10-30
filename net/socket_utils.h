#ifndef COMMON_LIBRARY_SOCKET_UTILS_H
#define COMMON_LIBRARY_SOCKET_UTILS_H

#include <stdint.h>

namespace common_library {

class SocketUtils {
  public:
    static int SetNonBlocked(int fd, bool nonblocked = true);

    static int Connect(const char* host,
                       uint16_t    port,
                       const char* local_ip,
                       uint16_t    local_port,
                       bool        async);
};
}  // namespace common_library

#endif