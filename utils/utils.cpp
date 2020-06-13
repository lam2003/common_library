#include <utils/once_token.h>
#include <utils/utils.h>

#include <atomic>
#include <chrono>
#include <thread>

#include <linux/limits.h>
#include <unistd.h>

namespace common_library {

static inline uint64_t get_current_microseconds_origin()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

static std::atomic<uint64_t>
    s_now_microseconds(get_current_microseconds_origin());
static std::atomic<uint64_t>
    s_now_milliseconds(get_current_microseconds_origin() / 1000);

static inline bool init_update_ts_thread()
{
    // C++11局部静态变量构造是线程安全的
    static std::thread update_ts_thread([]() {
        uint64_t now_microseconds;
        while (true) {
            now_microseconds = get_current_microseconds_origin();
            s_now_microseconds.store(now_microseconds,
                                     std::memory_order_release);
            s_now_milliseconds.store(now_microseconds / 1000,
                                     std::memory_order_release);
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
    });

    static OnceToken once_token([&]() { update_ts_thread.detach(); });

    return true;
}

uint64_t get_current_microseconds()
{
    static bool flag = init_update_ts_thread();
    return s_now_microseconds.load(std::memory_order_acquire);
}

uint64_t get_current_milliseconds()
{
    static bool flag = init_update_ts_thread();
    return s_now_milliseconds.load(std::memory_order_acquire);
}

std::string get_exe_path()
{
    std::string path;
    char        buf[PATH_MAX * 2 + 1] = {0};
    int         n = readlink("/proc/self/exe", buf, sizeof(buf));
    if (n <= 0) {
        path = "./";
    }
    else {
        path = buf;
    }
    return path;
}

std::string get_exe_name()
{
    std::string path = get_exe_path();
    return path.substr(path.rfind("/") + 1);
}

}  // namespace common_library