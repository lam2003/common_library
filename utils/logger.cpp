#include <utils/logger.h>
#include <utils/utils.h>

#include <sys/time.h>

namespace common_library {

INSTANCE_IMPL(Logger, get_exe_name());

Logger::Logger(const std::string& logger_name) {}

Logger::~Logger() {}

LogContext::LogContext(LogLevel    level,
                       const char* file,
                       const char* function,
                       int         line)
    : level_(level), file_(file), function_(function), line_(line)
{
    gettimeofday(&tv_, nullptr);
}
}  // namespace common_library