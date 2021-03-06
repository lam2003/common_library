#ifndef COMMON_LIBRARY_UTILS_H
#define COMMON_LIBRARY_UTILS_H

#include <memory>
#include <sstream>
#include <string>

#define INSTANCE_IMPL(class_name, ...)                                         \
    class_name& class_name::Instance()                                         \
    {                                                                          \
        static std::shared_ptr<class_name> s_instance(                         \
            new class_name(__VA_ARGS__));                                      \
        static class_name& s_instance_ref = *s_instance;                       \
        return s_instance_ref;                                                 \
    }

namespace common_library {

class __StringPrinter : public std::string {
  public:
    __StringPrinter()  = default;
    ~__StringPrinter() = default;

  public:
    template <typename T> __StringPrinter& operator<<(T&& data)
    {
        ss_ << std::forward<T>(data);
        std::string::operator=(ss_.str());
        return *this;
    }

    std::string operator<<(std::ostream& (*f)(std::ostream&)) const
    {
        return *this;
    }

  private:
    std::stringstream ss_;
};

#define StringPrinter __StringPrinter()

uint64_t get_current_microseconds();

uint64_t get_current_milliseconds();

uint64_t get_current_seconds();

std::string print_time(const timeval& tv);

std::string get_exe_path();

std::string get_exe_name();

enum ThreadPriority {
    TPRIORITY_LOWEST,
    TPRIORITY_LOW,
    TPRIORITY_NORMAL,
    TPRIORITY_HIGH,
    TPRIORITY_HIGHEST
};

bool set_thread_priority(ThreadPriority priority = TPRIORITY_NORMAL,
                         pthread_t      tid      = 0);

bool set_thread_name(const std::string& name);
}  // namespace common_library

#endif