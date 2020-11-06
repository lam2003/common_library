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

namespace common_library {

int SocketUtils::Connect(const char* host,
                         uint16_t    port,
                         const char* local_ip,
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

    int ret = -1;
    if (is_ipv6 && strcmp("0.0.0.0", local_ip) == 0) {
        ret = Bind(fd, "::", local_port, is_ipv6);
    }
    else {
        ret = Bind(fd, local_ip, local_port, is_ipv6);
    }

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

int SocketUtils::Bind(int fd, const char* local_ip, uint16_t port, bool is_ipv6)
{
    sockaddr* addr;
    socklen_t len;
    if (is_ipv6) {
        sockaddr_in6 in;
        bzero(&in, sizeof(in));
        in.sin6_family = AF_INET6;
        in.sin6_port   = htons(port);

        char buf[sizeof(in6_addr)];
        if (!inet_pton(AF_INET6, local_ip, buf)) {
            LOG_E << "socket bind failed. local_ip=" << local_ip
                  << " ,port=" << port;
            return -1;
        }

        memcpy(&in.sin6_addr, buf, sizeof(in6_addr));
        addr = reinterpret_cast<sockaddr*>(&in);
        len  = sizeof(in);
    }
    else {
        sockaddr_in in;
        bzero(&in, sizeof(in));
        in.sin_family = AF_INET;
        in.sin_port   = htons(port);

        char buf[sizeof(in_addr)];
        if (!inet_pton(AF_INET, local_ip, buf)) {
            LOG_E << "socket bind failed. local_ip=" << local_ip
                  << " ,port=" << port;
            return -1;
        }

        memcpy(&in.sin_addr, buf, sizeof(in_addr));
        addr = reinterpret_cast<sockaddr*>(&in);
        len  = sizeof(in);
    }

    int ret = ::bind(fd, addr, len);
    if (ret == -1) {
        LOG_E << "socket bind failed. " << get_uv_errmsg()
              << " ,local_ip=" << local_ip << " ,port=" << port;
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