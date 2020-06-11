#include <utils/once_token.h>
#include <utils/utils.h>

#include <atomic>
#include <chrono>
#include <thread>

#include <unistd.h>

namespace common_library {

static inline int64_t get_current_microseconds_origin()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

static std::atomic<int64_t>
    s_now_microseconds(get_current_microseconds_origin());
static std::atomic<int64_t>
    s_now_milliseconds(get_current_microseconds_origin() / 1000);

static inline bool init_update_ts_thread()
{
    // C++11局部静态变量构造是线程安全的
    static std::thread update_ts_thread([]() {
        int64_t now_microseconds;
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

int64_t get_current_microseconds()
{
    static bool flag = init_update_ts_thread();
    return s_now_microseconds.load(std::memory_order_acquire);
}

int64_t get_current_milliseconds()
{
    static bool flag = init_update_ts_thread();
    return s_now_milliseconds.load(std::memory_order_acquire);
}

}  // namespace common_library