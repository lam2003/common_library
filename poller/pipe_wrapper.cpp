#include <net/socket_utils.h>
#include <poller/pipe_wrapper.h>
#include <utils/utils.h>
#include <utils/uv_error.h>

#include <stdexcept>

#include <unistd.h>

#define CLOSE_FD(fd)                                                           \
    if (fd != -1) {                                                            \
        ::close(fd);                                                           \
        fd = -1;                                                               \
    }

namespace common_library {

PipeWrapper::PipeWrapper()
{
    if (pipe(pipe_fd_) == -1) {
        throw std::runtime_error(StringPrinter << "create pipe failed. "
                                               << get_uv_errmsg());
    }

    set_socket_non_blocked(pipe_fd_[0], true);
    set_socket_non_blocked(pipe_fd_[1], false);
}

PipeWrapper::~PipeWrapper()
{
    close_fd();
}

int PipeWrapper::Write(const void* buf, int n)
{
    int ret;
    do {
        ret = ::write(pipe_fd_[1], buf, n);

    } while (ret == -1 && get_uv_error() == EINTR);
    return ret;
}

int PipeWrapper::Read(void* buf, int n)
{
    int ret;
    do {
        ret = ::read(pipe_fd_[0], buf, n);

    } while (ret == -1 && get_uv_error() == EINTR);
    return ret;
}

int PipeWrapper::ReadFD()
{
    return pipe_fd_[0];
}

int PipeWrapper::WriteFD()
{
    return pipe_fd_[1];
}

void PipeWrapper::close_fd()
{
    CLOSE_FD(pipe_fd_[0]);
    CLOSE_FD(pipe_fd_[1]);
}

}  // namespace common_library