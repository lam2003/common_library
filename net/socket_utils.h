#ifndef COMMON_LIBRARY_SOCKET_UTILS_H
#define COMMON_LIBRARY_SOCKET_UTILS_H

namespace common_library {

int set_socket_non_blocked(int fd, bool nonblocked = true);

}  // namespace common_library

#endif