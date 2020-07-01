#ifndef COMMON_LIBRARY_BUFFER_H
#define COMMON_LIBRARY_BUFFER_H

#include <utils/noncopyable.h>

#include <memory>
#include <string>

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

}  // namespace common_library

#endif