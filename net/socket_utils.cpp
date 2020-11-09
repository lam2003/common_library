#include <net/dns_cache.h>
#include <net/socket_utils.h>
#include <utils/logger.h>
#include <utils/uv_error.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <ifaddrs.h>
#include <net/if.h>

namespace common_library {

int SocketUtils::Connect(const char* host,
                         uint16_t    port,
                         const char* local_ip_or_intf,
                         uint16_t    local_port,
                         bool        async)
{
    sockaddr_storage addr;
    DNSCache::Instance().Parse(host, addr);

    bool is_ipv6 = false;
    int  fd      = -1;

    switch (addr.ss_family) {
        case AF_INET6: {
            fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
            if (fd < 0) {
                LOG_E << "create ipv6 socket failed";
                return -1;
            }
            {
                char          strbuf[INET6_ADDRSTRLEN];
                sockaddr_in6* in    = reinterpret_cast<sockaddr_in6*>(&addr);
                in->sin6_port       = htons(port);
                std::string straddr = inet_ntop(AF_INET6, &in->sin6_addr,
                                                strbuf, INET6_ADDRSTRLEN);
                LOG_I << "dns ok. [ipv6] " << host << "-->" << straddr;
            }
            is_ipv6 = true;
            break;
        }
        case AF_INET: {
            fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (fd < 0) {
                LOG_E << "create ipv4 socket failed";
                return -1;
            }
            {
                char         strbuf[INET_ADDRSTRLEN];
                sockaddr_in* in = reinterpret_cast<sockaddr_in*>(&addr);
                in->sin_port    = htons(port);
                std::string straddr =
                    inet_ntop(AF_INET, &in->sin_addr, strbuf, INET_ADDRSTRLEN);
                LOG_I << "dns ok. [ipv4] " << host << "-->" << straddr;
            }
            break;
        }
        default: {
            LOG_E << "connect failed. unknow socket family";
            return -1;
        }
    }

    SetReuseable(fd);
    SetNoBlocked(fd, async);
    SetNoDelay(fd);
    SetRecvBuf(fd);
    SetSendBuf(fd);
    SetCloseWait(fd);
    SetCloExec(fd);

    int ret = Bind(fd, local_ip_or_intf, local_port, is_ipv6);
    if (ret == -1) {
        ::close(fd);
        return ret;
    }

    ret = ::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (ret == 0) {
        // 同步连接成功
        return fd;
    }

    if (async && get_uv_error() == EAGAIN) {
        // 异步连接成功
        return fd;
    }

    LOG_E << "connect to " << host << " " << port << " failed. "
          << get_uv_errmsg();

    ::close(fd);

    return -1;
}

static int get_addr_by_if(int         family,
                          const char* adapter_name,
                          uint16_t    port,
                          sockaddr*   addr)
{
    ifaddrs* list = nullptr;
    int      ret  = getifaddrs(&list);
    if (ret == -1) {
        LOG_E << "getifaddrs failed. " << get_uv_errmsg();
        return ret;
    }

    ifaddrs* cur   = list;
    ifaddrs* found = nullptr;
    // 先按网卡名查找
    while (cur) {
        if (cur->ifa_addr && cur->ifa_name &&
            family == cur->ifa_addr->sa_family) {
            if (strcmp(cur->ifa_name, adapter_name) == 0) {
                found = cur;
                break;
            }
        }
        cur = cur->ifa_next;
    }

    // 按ip地址进行匹配
    if (!found && (family == AF_INET || family == AF_INET6)) {
        uint8_t addrbytes[sizeof(in6_addr)];
        int     comparesize =
            (family == AF_INET) ? sizeof(in_addr) : sizeof(in6_addr);

        void* cmp = nullptr;
        if (inet_pton(family, adapter_name, addrbytes) == 1) {
            cur = list;
            while (cur) {
                if (cur->ifa_addr && family == cur->ifa_addr->sa_family) {
                    if (family == AF_INET) {
                        sockaddr_in* addr4 =
                            reinterpret_cast<sockaddr_in*>(cur->ifa_addr);
                        cmp = &(addr4->sin_addr);
                    }
                    else {
                        sockaddr_in6* addr6 =
                            reinterpret_cast<sockaddr_in6*>(cur->ifa_addr);
                        cmp = &(addr6->sin6_addr);
                    }

                    if (memcmp(cmp, addrbytes, comparesize) == 0) {
                        found = cur;
                        break;
                    }
                }
                cur = cur->ifa_next;
            }
        }
    }

    if (!found) {
        if (family == AF_INET6 && strcmp(adapter_name, "0.0.0.0") == 0) {
            (reinterpret_cast<sockaddr_in6*>(addr))->sin6_port   = htons(port);
            (reinterpret_cast<sockaddr_in6*>(addr))->sin6_family = AF_INET6;
            if (!inet_pton(
                    AF_INET6, "::",
                    &(reinterpret_cast<sockaddr_in6*>(addr))->sin6_addr)) {
                return -1;
            }
            return 0;
        }
        else if (family == AF_INET && strcmp(adapter_name, "0.0.0.0") == 0) {
            (reinterpret_cast<sockaddr_in*>(addr))->sin_port   = htons(port);
            (reinterpret_cast<sockaddr_in*>(addr))->sin_family = AF_INET;
            if (!inet_pton(AF_INET, "0.0.0.0",
                           &(reinterpret_cast<sockaddr_in*>(addr))->sin_addr)) {
                return -1;
            }
            return 0;
        }
        else {
            return -1;
        }
    }

    if (family == AF_INET6) {
        *(reinterpret_cast<sockaddr_in6*>(addr)) =
            *(reinterpret_cast<sockaddr_in6*>(found->ifa_addr));
        (reinterpret_cast<sockaddr_in6*>(addr))->sin6_port = htons(port);
    }
    else {
        *(reinterpret_cast<sockaddr_in*>(addr)) =
            *(reinterpret_cast<sockaddr_in*>(found->ifa_addr));
        (reinterpret_cast<sockaddr_in*>(addr))->sin_port = htons(port);
    }

    return 0;
}

int SocketUtils::Bind(int         fd,
                      const char* local_ip_or_intf,
                      uint16_t    port,
                      bool        is_ipv6)
{
    int       family;
    sockaddr* addr;
    socklen_t len;

    if (is_ipv6) {
        family = AF_INET6;
        sockaddr_in6* in =
            reinterpret_cast<sockaddr_in6*>(malloc(sizeof(sockaddr_in6)));
        bzero(in, sizeof(sockaddr_in6));
        addr = reinterpret_cast<sockaddr*>(in);
        len  = sizeof(sockaddr_in6);
    }
    else {
        family = AF_INET;
        sockaddr_in* in =
            reinterpret_cast<sockaddr_in*>(malloc(sizeof(sockaddr_in)));
        bzero(in, sizeof(sockaddr_in));
        addr = reinterpret_cast<sockaddr*>(in);
        len  = sizeof(sockaddr_in);
    }

    int ret = get_addr_by_if(family, local_ip_or_intf, port, addr);
    if (ret == -1) {
        free(addr);
        LOG_W << "get address by interface failed. "
              << "local_ip_or_intf=" << local_ip_or_intf;

        return ret;
    }

    ret = ::bind(fd, addr, len);
    free(addr);

    if (ret == -1) {
        LOG_E << "socket bind failed. " << get_uv_errmsg()
              << ", local_ip_or_intf=" << local_ip_or_intf << ", port=" << port;
    }

    return ret;
}

int SocketUtils::SetNoBlocked(int fd, bool enable)
{
    int opt = enable;

    int ret = ioctl(fd, FIONBIO, &opt);
    if (ret == -1) {
        LOG_E << "set noblocked failed. " << get_uv_errmsg() << " ,fd=" << fd;
        return ret;
    }

    return ret;
}

int SocketUtils::SetReuseable(int fd, bool enable)
{
    int opt = enable;

    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret == -1) {
        LOG_E << "set SO_REUSEADDR failed. " << get_uv_errmsg()
              << ", fd=" << fd;
        return ret;
    }

    return ret;
}

int SocketUtils::SetKeepAlive(int fd, bool enable)
{
    int opt = enable;

    int ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
    if (ret == -1) {
        LOG_E << "set SO_KEEPALIVE failed. " << get_uv_errmsg()
              << ", fd=" << fd;
        return ret;
    }

    return ret;
}

int SocketUtils::SetNoDelay(int fd, bool enable)
{
    int opt = enable;

    int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
    if (ret == -1) {
        LOG_E << "set SO_KEEPALIVE failed. " << get_uv_errmsg()
              << ", fd=" << fd;
        return ret;
    }

    return ret;
}

int SocketUtils::SetSendBuf(int fd, int size)
{
    int ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
    if (ret == -1) {
        LOG_E << "set SO_SNDBUF failed. " << get_uv_errmsg() << ", fd=" << fd
              << ", size=" << size;
        return ret;
    }

    return ret;
}

int SocketUtils::SetRecvBuf(int fd, int size)
{
    int ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    if (ret == -1) {
        LOG_E << "set SO_RCVBUF failed. " << get_uv_errmsg() << ", fd=" << fd
              << ", size=" << size;
        return ret;
    }

    return ret;
}

int SocketUtils::SetCloExec(int fd, bool enable)
{
    int flags = fcntl(fd, F_GETFD);
    if (flags == -1) {
        LOG_E << "set FD_CLOEXEC failed. " << get_uv_errmsg() << ", fd=" << fd;
        return -1;
    }

    int f = FD_CLOEXEC;
    if (enable) {
        flags |= f;
    }
    else {
        flags &= ~f;
    }

    int ret = fcntl(fd, F_SETFD, flags);
    if (ret == -1) {
        LOG_E << "set FD_CLOEXEC failed. " << get_uv_errmsg() << ", fd=" << fd;
        return ret;
    }

    return ret;
}

int SocketUtils::SetCloseWait(int fd, int seconds)
{
    linger l;
    // 在调用closesocket()时还有数据未发送完，允许等待
    // l.l_onoff=0;则调用closesocket()后强制关闭
    l.l_onoff = (seconds > 0);
    // 设置等待时间为x秒
    l.l_linger = seconds;
    int ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&l, sizeof(linger));
    if (ret == -1) {
        LOG_E << "set SO_LINGER failed. " << get_uv_errmsg() << ", fd=" << fd
              << " ,seconds=" << seconds;
        return ret;
    }
    return ret;
}

int SocketUtils::SetSendTimeout(int fd, int seconds)
{
    struct timeval timeout;
    timeout.tv_sec  = seconds;
    timeout.tv_usec = 0;

    int ret =
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    if (ret == -1) {
        LOG_E << "set SO_SNDTIMEO failed. " << get_uv_errmsg() << " ,fd=" << fd
              << " ,seconds=" << seconds;
        return ret;
    }
    return ret;
}
}  // namespace common_library