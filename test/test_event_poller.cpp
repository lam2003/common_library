#include "poller/event_poller.h"
#include "utils/logger.h"
#include "utils/time_ticker.h"
#include "utils/utils.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>

using namespace std;
using namespace common_library;

/**
 * cpu负载均衡测试
 * @return
 */
int main()
{
    static bool exit_flag = false;
    signal(SIGINT, [](int) { exit_flag = true; });
    //设置日志
    Logger::Instance().AddChannel(std::make_shared<ConsoleChannel>());
    Logger::Instance().SetWriter(
        std::make_shared<AsyncLogWriter>(Logger::Instance()));
    Ticker ticker;
    while (!exit_flag) {
   
        EventPollerPool::Instance().GetExecutor()->Async([]() {
            auto usec = rand() % 4000;
            // DebugL << usec;
            usleep(usec);
        });

        usleep(2000);
    }

    return 0;
}
