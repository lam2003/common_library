#ifndef COMMON_LIBRARY_UTILS_H
#define COMMON_LIBRARY_UTILS_H

#include <string>

#define INSTANCE_IMPL(class_name, ...)                                         \
    class_name& class_name::Instance()                                         \
    {                                                                          \
        static std::shared_ptr<class_name> instance(                           \
            new class_name(__VA_ARGS__));                                      \
        static class_name& instance_ref = *instance;                           \
        return instance_ref;                                                   \
    }

namespace common_library {

uint64_t get_current_microseconds();

uint64_t get_current_milliseconds();

std::string get_exe_path();

std::string get_exe_name();

}  // namespace common_library

#endif