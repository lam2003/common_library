#include <atomic>
#include <iostream>
#include <signal.h>
#include <unistd.h>

#include "thread/pool.h"

using namespace std;
using namespace common_library;

int main()
{
    signal(SIGINT, [](int) { exit(0); });

    atomic_llong count(0);
    ThreadPool   pool(1, ThreadPool::PRIORITY_HIGHEST, false);

    uint64_t tp1 = get_current_milliseconds();
    uint64_t tp2, tp3, tp4;

    for (int i = 0; i < 1000 * 10000; ++i) {
        pool.Async([&]() {
            if (++count >= 1000 * 10000) {
                tp4 = get_current_milliseconds();
                LOG_I << "执行1000万任务总共耗时:" << (tp4 - tp3) << "ms"
                      << endl;
            }
        });
    }
    tp2 = get_current_milliseconds();
    LOG_I << "1000万任务入队耗时:" << (tp2 - tp1) << "ms" << endl;
    uint64_t lastCount = 0, nowCount = 1;

    tp3 = get_current_milliseconds();
    //此处才开始启动线程
    pool.Start();
    while (true) {
        sleep(1);
        nowCount = count.load();
        LOG_I << "每秒执行任务数:" << nowCount - lastCount << endl;
        if (nowCount - lastCount == 0) {
            break;
        }
        lastCount = nowCount;
    }

    pool.Shutdown();
    pool.Wait();
    return 0;
}
