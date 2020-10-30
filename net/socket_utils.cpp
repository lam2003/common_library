#include <net/socket_utils.h>
#include <utils/logger.h>
#include <utils/uv_error.h>

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace common_library {

int SocketUtils::SetNonBlocked(int fd, bool non_blocked)
{
    int opt = non_blocked;

    int ret = ioctl(fd, FIONBIO, &opt);
    if (ret == -1) {
        LOG_E << "set nonblocked failed. " << get_uv_errmsg() << " ,fd=" << fd;
        return ret;
    }

    return ret;
}

int SocketUtils::Connect(const char* host,
                         uint16_t    port,
                         const char* local_ip,
                         uint16_t    local_port,
                         bool        async)
{
    // sockaddr_in
    return 0;
}

}  // namespace common_library