#ifndef COMMON_LIBRARY_TASK_EXECUTOR_H
#define COMMON_LIBRARY_TASK_EXECUTOR_H

#include <thread/semaphore.h>
#include <utils/noncopyable.h>
#include <utils/once_token.h>

#include <functional>
#include <memory>

namespace common_library {

class TaskCancelable : public noncopyable {
public:
  TaskCancelable() = default;
  virtual ~TaskCancelable() = default;
  virtual void Cancel() = 0;
};

template <typename T> class TaskCancelableImpl;

template <typename Ret, typename... Args>
class TaskCancelableImpl<Ret(Args...)> : public TaskCancelable {
public:
  typedef std::shared_ptr<TaskCancelableImpl> ptr;
  typedef std::function<Ret(Args...)> function_type;

  ~TaskCancelableImpl() = default;

  template <typename Func> TaskCancelableImpl(Func &&f) {
    strong_task_ = std::make_shared<function_type>(std::forward<Func>(f));
    weak_task_ = strong_task_;
  }

  void Cancel() override { strong_task_ = nullptr; }

  // return cancelable
  operator bool() { return strong_task_ && *strong_task_; }

  Ret operator()(Args... args) const {
    std::shared_ptr<function_type> strong_task = weak_task_.lock();
    if (strong_task && *strong_task) {
      return (*strong_task)(std::forward<Args>(args)...);
    }
    return default_return<Ret>();
  }

protected:
  template <typename T>
  static typename std::enable_if<std::is_void<T>::value, void>::type
  default_return() {}

  template <typename T>
  static typename std::enable_if<std::is_integral<T>::value, T>::type
  default_return() {
    return 0;
  }

  template <typename T>
  static typename std::enable_if<std::is_pointer<T>::value, T>::type
  default_return() {
    return nullptr;
  }

private:
  std::shared_ptr<function_type> strong_task_;
  std::weak_ptr<function_type> weak_task_;
};

typedef std::function<void()> TaskIn;
typedef TaskCancelableImpl<void()> Task;

class TaskExecutorInterface {
public:
  TaskExecutorInterface() = default;
  virtual ~TaskExecutorInterface() = default;

  virtual Task::ptr Async(TaskIn &&task, bool may_sync = true) = 0;
  virtual Task::ptr AsyncFirst(TaskIn &&task, bool may_sync = true) {
    return Async(std::move(task), may_sync);
  }

  void Sync(TaskIn &&task) {
    Semaphore sem;
    std::shared_ptr<Task> ptask = Async([&]() {
      OnceToken(nullptr, [&]() {
        // 利用RAII原理避免发生异常时这行代码未被执行
        sem.Post();
      });
      task();
    });

    if (ptask && *ptask) {
      sem.Wait();
    }
  }

  void SyncFirst(TaskIn &&task, bool may_sync = true) {
    Semaphore sem;
    std::shared_ptr<Task> ptask = AsyncFirst([&]() {
      OnceToken(nullptr, [&]() {
        // 利用RAII原理避免发生异常时这行代码未被执行
        sem.Post();
      });
      task();
    });

    if (ptask && *ptask) {
      sem.Wait();
    }
  }
};

class ThreadLoadCounter {};

} // namespace common_library

#endif