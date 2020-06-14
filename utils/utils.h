#ifndef COMMON_LIBRARY_UTILS_H
#define COMMON_LIBRARY_UTILS_H

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

uint64_t get_current_microseconds();

uint64_t get_current_milliseconds();

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

bool set_thread_priority(ThreadPriority priority);

}  // namespace common_library

#endif