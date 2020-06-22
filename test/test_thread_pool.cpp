#include "thread/pool.h"
#include "utils//logger.h"
#include "utils/time_ticker.h"
#include <atomic>
#include <iostream>
#include <signal.h>
#include <unistd.h>

using namespace std;
using namespace common_library;

int main()
{
    signal(SIGINT, [](int) { exit(0); });
    //初始化日志系统
    Logger::Instance().AddChannel(std::make_shared<ConsoleChannel>());
    Logger::Instance().SetWriter(
        std::make_shared<AsyncLogWriter>(Logger::Instance()));

    atomic_llong count(0);
    ThreadPool   pool(1, TPRIORITY_HIGHEST, false);

    Ticker ticker;
    for (int i = 0; i < 1000 * 10000; ++i) {
        pool.Async([&]() {
            if (++count >= 1000 * 10000) {
                LOG_I << "执行1000万任务总共耗时:" << ticker.ElapsedTimeMS()
                      << "ms";
            }
        });
    }
    LOG_I << "1000万任务入队耗时:" << ticker.ElapsedTimeMS() << "ms" << endl;
    uint64_t lastCount = 0, nowCount = 1;
    ticker.Reset();
    //此处才开始启动线程
    pool.Start();
    while (true) {
        sleep(1);
        nowCount = count.load();
        LOG_I << "每秒执行任务数:" << nowCount - lastCount;
        if (nowCount - lastCount == 0) {
            break;
        }
        lastCount = nowCount;
    }
    return 0;
}
