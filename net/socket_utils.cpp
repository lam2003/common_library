#include <net/socket_utils.h>
#include <utils/logger.h>
#include <utils/uv_error.h>

#include <sys/ioctl.h>

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

}  // namespace common_library