#ifndef COMMON_LIBRARY_LOGGER_H
#define COMMON_LIBRARY_LOGGER_H

#include <utils/noncopyable.h>

#include <iostream>
#include <sstream>
#include <memory>

namespace common_library {

typedef enum { LTRACE, LDEBUG, LINFO, LWARN, LERROR } LogLevel;

class AsyncLogWriter;

class Logger final : public noncopyable,
                     public std::enable_shared_from_this<Logger> {
  public:
    friend class AsyncLogWriter;

    ~Logger();
    static Logger& Instance();

  private:
    Logger(const std::string& logger_name);

  private:
};

class LogContext final : public std::ostringstream {
  public:
    LogContext(LogLevel    level,
               const char* file,
               const char* function,
               int         line);
    ~LogContext() = default;

  private:
    LogLevel    level_;
    int         line_;
    std::string file_;
    std::string function_;
    timeval     tv_;
};

class LogWriter : public noncopyable {
  public:
    LogWriter();
    virtual ~LogWriter();

  public:
    virtual void Write(const LogContext& ctx) = 0;
};

class AsyncLogWrite final : public LogWriter {
  public:
    AsyncLogWrite();
    ~AsyncLogWrite();

  private:
    void Write(const LogContext& ctx) override;
};

// TODO implement logger
#define LOG_T std::cout
#define LOG_D std::cout
#define LOG_I std::cout
#define LOG_W std::cout
#define LOG_E std::cout

}  // namespace common_library

#endif