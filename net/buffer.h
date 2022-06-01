#ifndef COMMON_LIBRARY_BUFFER_H
#define COMMON_LIBRARY_BUFFER_H

#include <utils/list.h>
#include <utils/noncopyable.h>

#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace common_library {

class Buffer : public noncopyable {
  public:
    typedef std::shared_ptr<Buffer> Ptr;
    Buffer() {}
    virtual ~Buffer() {}

    virtual char*       Data() const = 0;
    virtual uint32_t    Size() const = 0;
    virtual std::string ToString() const
    {
        return std::string(Data(), Size());
    }
    virtual uint32_t Capacity() const
    {
        return Size();
    }
};

class BufferRaw final : public Buffer {
  public:
    typedef std::shared_ptr<BufferRaw> Ptr;
    BufferRaw(uint32_t capacity = 0)
    {
        if (capacity) {
            SetCapacity(capacity);
        }
    }
    ~BufferRaw()
    {
        if (data_) {
            delete[] data_;
        }
    }

  public:
    char* Data() const override
    {
        return data_;
    }
    uint32_t Size() const override
    {
        return size_;
    }

    uint32_t Capacity() const override
    {
        return capacity_;
    }

  public:
    void SetCapacity(uint32_t capacity)
    {
        if (data_) {
            do {
                if (capacity > capacity_) {
                    // 需要重新分配内存
                    break;
                }

                // 例如：1500 * 2 = 3000 > 2048
                // 此时没必要重新分配，一页内存4096KB
                if (capacity < 2 * 1024) {
                    // 2k以下，不需要重复开辟
                    return;
                }

                if (capacity * 2 > capacity_) {
                    return;
                }

            } while (0);

            delete[] data_;
        }
        data_     = new char[capacity];
        capacity_ = capacity;
    }

    void SetSize(uint32_t size)
    {
        if (size > capacity_) {
            throw std::invalid_argument("Buffer::SetSize out of range");
        }
        size_ = size;
    }

    void Assign(const char* data, uint32_t size = 0)
    {
        if (size <= 0) {
            // 字符串
            size = strlen(data);
        }

        SetCapacity(size + 1);
        memcpy(data_, data, size);
        data_[size] = '\0';
        SetSize(size);
    }

  private:
    char*    data_     = nullptr;
    uint32_t capacity_ = 0;
    uint32_t size_     = 0;
};

class BufferList;
class BufferSock : public Buffer {
  public:
    friend class BufferList;
    BufferSock(const Buffer::Ptr&       buffer,
               struct sockaddr_storage* addr     = nullptr,
               socklen_t                addr_len = 0);
    ~BufferSock();

  public:
    char*    Data() const override;
    uint32_t Size() const override;

  private:
    Buffer::Ptr              buffer_;
    struct sockaddr_storage* addr_     = nullptr;
    socklen_t                addr_len_ = 0;
};

class BufferList : public noncopyable {
  public:
    typedef std::shared_ptr<BufferList> Ptr;
    BufferList(List<Buffer::Ptr>& list);
    ~BufferList() {}

  public:
    bool empty();
    int  count();
    int  send(int fd, int flags, bool udp);

  private:
    void reoffset(int n);
    int  send_l(int fd, int flags, bool udp);

  private:
    std::vector<struct iovec> iovec_;
    int                       iovec_off_   = 0;
    int                       remain_size_ = 0;
    List<Buffer::Ptr>         pkt_list_;
};
}  // namespace common_library

#endif