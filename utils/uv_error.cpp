#include <utils/uv_error.h>

#include <string.h>

namespace common_library {

int uv_translate_posix_error(int err)
{
    switch (err) {
        case ENOBUFS:
        case EINPROGRESS:
        case EWOULDBLOCK: {
            err = EAGAIN;
            break;
        }
        default: {
            break;
        }
    }

    return err;
}

int get_uv_error()
{
    return uv_translate_posix_error(errno);
}

const char* get_uv_errmsg()
{
    return strerror(get_uv_error());
}

const char* uv_strerror(int err)
{
    return strerror(err);
}
}  // namespace common_library