#include <net/buffer.h>
#include <utils/uv_error.h>

#include <limits.h>
#include <sys/uio.h>
namespace common_library {

BufferSock::BufferSock(const Buffer::Ptr& buffer,
                       struct sockaddr*   addr,
                       socklen_t          addr_len)
{
    if (addr && addr_len) {
        addr_ = (struct sockaddr*)malloc(addr_len);
        memcpy(addr_, addr, addr_len);
        addr_len_ = addr_len;
    }
    buffer_ = buffer;
}

BufferSock::~BufferSock()
{
    if (addr_) {
        free(addr_);
    }
}

char* BufferSock::Data() const
{
    return buffer_->Data();
}

uint32_t BufferSock::Size() const
{
    return buffer_->Size();
}

BufferList::BufferList(List<Buffer::Ptr>& list)
{
    pkt_list_.swap(list);
    std::vector<struct iovec>::iterator it = iovec_.begin();

    pkt_list_.for_each([&](const Buffer::Ptr& buffer) {
        it->iov_base = buffer->Data();
        it->iov_len  = buffer->Size();
        remain_size_ += it->iov_len;
        it++;
    });
}

bool BufferList::empty()
{
    return iovec_off_ == static_cast<int>(iovec_.size());
}

int BufferList::count()
{
    return iovec_.size() - iovec_off_;
}

int BufferList::send(int fd, int flags, bool udp)
{
    int remain_size = remain_size_;
    while (remain_size_ && send_l(fd, flags, udp) != -1) {}

    int sent = remain_size - remain_size_;
    if (sent > 0) {
        return sent;
    }
    return -1;
}

void BufferList::reoffset(int n)
{
    remain_size_ -= n;
    int offset   = 0;
    int last_off = iovec_off_;

    for (int i = last_off; i != static_cast<int>(iovec_.size()); i++) {
        struct iovec& ref = iovec_[i];
        offset += ref.iov_len;
        if (offset < n) {
            continue;
        }

        int remain_size = offset - n;
        ref.iov_base    = (char*)ref.iov_base + ref.iov_len - remain_size;
        ref.iov_len     = remain_size;
        iovec_off_      = i;
        if (remain_size == 0) {
            iovec_off_ += 1;
        }
        break;
    }

    for (int i = 0; i < iovec_off_; i++) {
        pkt_list_.pop_front();
    }
}

int BufferList::send_l(int fd, int flags, bool udp)
{
    int n;

    do {
        struct msghdr msg;
        if (!udp) {
            msg.msg_name    = nullptr;
            msg.msg_namelen = 0;
        }
        else {
            BufferSock* buffer =
                static_cast<BufferSock*>(pkt_list_.front().get());
            msg.msg_name    = buffer->addr_;
            msg.msg_namelen = buffer->addr_len_;
        }

        msg.msg_iov    = &iovec_[iovec_off_];
        msg.msg_iovlen = iovec_.size() - iovec_off_;
        int max        = udp ? 1 : IOV_MAX;
        if (msg.msg_iovlen > static_cast<size_t>(max)) {
            msg.msg_iovlen = max;
        }

        msg.msg_control    = nullptr;
        msg.msg_controllen = 0;
        msg.msg_flags      = flags;
        n                  = sendmsg(fd, &msg, flags);

    } while (n == -1 && EINTR == get_uv_error());

    if (n >= remain_size_) {
        iovec_off_   = iovec_.size();
        remain_size_ = 0;
        return n;
    }

    if (n > 0) {
        reoffset(n);
        return n;
    }

    return n;
}
}  // namespace common_library