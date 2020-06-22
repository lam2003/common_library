#ifndef COMMON_LIBRARY_SOCKET_UTILS_H
#define COMMON_LIBRARY_SOCKET_UTILS_H

namespace common_library {

class SocketUtils {
  public:
    static int SetNonBlocked(int fd, bool nonblocked = true);
};

}  // namespace common_library

#endif