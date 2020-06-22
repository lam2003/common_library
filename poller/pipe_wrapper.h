#ifndef COMMON_LIBRARY_PIPE_WRAPPER_H
#define COMMON_LIBRARY_PIPE_WRAPPER_H

namespace common_library {

class PipeWrapper final {
  public:
    PipeWrapper();

    ~PipeWrapper();

  public:
    int Write(const void* buf, int n);

    int Read(void* buf, int n);

    int ReadFD();

    int WriteFD();

  private:
    void close_fd();

  private:
    int pipe_fd_[2] = {-1, -1};
};

}  // namespace common_library

#endif