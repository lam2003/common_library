#ifndef COMMON_LIBRARY_UV_ERROR_H
#define COMMON_LIBRARY_UV_ERROR_H

#include <errno.h>

namespace common_library {
int         uv_translate_posix_error(int err);
int         get_uv_error();
const char* get_uv_errmsg();
}  // namespace common_library

#endif