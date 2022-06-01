#ifndef COMMON_LIBRARY_NONCOPYABLE_H
#define COMMON_LIBRARY_NONCOPYABLE_H

namespace common_library {
class noncopyable {
  protected:
    // 定义为protected原因是：noncopyable只有在被继承时才有意义，
    // 防止其被单独实例化
    noncopyable() {}
    ~noncopyable() {}

  private:
    // 禁止拷贝
    noncopyable(const noncopyable& that) = delete;
    noncopyable(noncopyable&& that)      = delete;
    noncopyable& operator=(const noncopyable& that) = delete;
    noncopyable& operator=(noncopyable&& that) = delete;
};

}  // namespace common_library

#endif