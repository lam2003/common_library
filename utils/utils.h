#ifndef COMMON_LIBRARY_UTILS_H
#define COMMON_LIBRARY_UTILS_H

#include <stdint.h>

namespace common_library {
uint64_t get_current_microseconds();
uint64_t get_current_milliseconds();
}  // namespace common_library

#endif