#ifndef COMMON_LIBRARY_ONCE_TOKEN_H
#define COMMON_LIBRARY_ONCE_TOKEN_H

#include <functional>

namespace common_library {

class OnceToken {
public:
  typedef std::function<void()> Task;
  OnceToken(const Task &on_constructed, const Task &on_destructed = nullptr) {
    if (on_constructed) {
      on_constructed();
    }
    on_destructed_ = on_destructed;
  }

  OnceToken(Task &&on_constructed, Task &&on_destructed = nullptr) {
    if (on_constructed) {
      on_constructed();
    }
    on_destructed_ = std::move(on_destructed);
  }

  ~OnceToken() {
    if (on_destructed_) {
      on_destructed_();
    }
  }

private:
  // 禁用默认构造哦函数及复制构造函数
  OnceToken() = delete;
  OnceToken(const OnceToken &that) = delete;
  OnceToken(OnceToken &&that) = delete;
  OnceToken operator=(const OnceToken &that) = delete;
  OnceToken operator=(OnceToken &&that) = delete;

private:
  Task on_destructed_;
};
} // namespace common_library

#endif