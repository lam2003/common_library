#include "thread/pool.h"
#include "utils/logger.h"
#include <atomic>
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <utils/utils.h>

using namespace std;
using namespace common_library;

int main()
{
    signal(SIGINT, [](int) { exit(0); });

    atomic_llong count(0);
    ThreadPool   pool(10, ThreadPool::PRIORITY_HIGHEST, false);

    // Ticker ticker;
    uint64_t tp1 = get_current_microseconds();

    for (int i = 0; i < 1000 * 10000; ++i) {
        pool.Async([&]() {
            if (++count >= 1000 * 10000) {
                uint64_t tp2 = get_current_microseconds();
                std::cout << "执行1000万任务总共耗时:" << ((tp2 - tp1) / 1000)
                          << "ms" << std::endl;
            }
        });
    }

    uint64_t tp3 = get_current_microseconds();

    std::cout << "1000万任务入队耗时:" << ((tp3 - tp1) / 1000) << "ms"
              << std::endl;

    uint64_t lastCount = 0, nowCount = 1;

    uint64_t tp4 = get_current_milliseconds();

    pool.Start();
    while (true) {
        sleep(1);
        nowCount = count.load();
        std::cout << "每秒执行任务数:" << nowCount - lastCount << std::endl;
        std::cout << "LOAD:" << pool.Load() << std::endl;
        if (nowCount - lastCount == 0) {
            break;
        }
        lastCount = nowCount;
    }

    pool.Shutdown();
    pool.Wait();
    return 0;
}
