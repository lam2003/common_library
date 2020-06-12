#include "thread/pool.h"
#include "utils/logger.h"
#include <atomic>
#include <iostream>
#include <signal.h>
#include <thread/group.h>
#include <thread/pool.h>
#include <thread/semaphore.h>
#include <thread/task.h>
#include <thread/task_executor.h>
#include <thread/task_queue.h>
#include <unistd.h>
#include <utils/function_traits.h>
#include <utils/utils.h>
#include <utils/list.h>
#include <utils/once_token.h>
using namespace std;
using namespace common_library;

int main()
{
    signal(SIGINT, [](int) { exit(0); });

    atomic_llong count(0);
    ThreadPool   pool(3, ThreadPool::PRIORITY_HIGHEST, true);

    // Ticker ticker;
    uint64_t tp1 = get_current_microseconds();

    for (int i = 0; i < 1000; ++i) {
        usleep(200 * 1000);
        pool.Async([&]() {
            // printf("aaa:%lld\n",pthread_self());
            // if (++count >= 1000 * 10000) {
            //     uint64_t tp2 = get_current_microseconds();
            //     std::cout << "执行1000万任务总共耗时:" << ((tp2 - tp1) /
            //     1000)
            //               << "ms" << std::endl;
            // }
            usleep(800 * 1000);
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
