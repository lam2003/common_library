#include "net/socket.h"
#include "net/socket_utils.h"
#include "poller/event_poller.h"
#include "thread/worker.h"
#include "utils/logger.h"
#include "utils/time_ticker.h"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;
using namespace common_library;

#define TEST_TIME 100
/**
 * socket工具库测试
 * @return
 */

int main()
{
    Semaphore sem;
    //设置日志
    Logger::Instance().AddChannel(std::make_shared<ConsoleChannel>());
    Logger::Instance().SetWriter(
        std::make_shared<AsyncLogWriter>(Logger::Instance()));

    TimeTicker();

    EventPollerPool::SetPoolSize(2);

    Socket::Ptr socket[TEST_TIME];
    for (int i = 0; i < TEST_TIME; i++) {
        EventPoller::Ptr poller = EventPollerPool::Instance().GetPoller();
        socket[i]               = std::make_shared<Socket>(poller, true);
    }

    for (int j = 0; j < 10; j++) {
        for (int i = 0; i < TEST_TIME; i++) {
            WorkerPool::Instance().GetWorker()->Async([i, &socket, &sem]() {
                socket[i]->Connect(
                    "linmin.xyz", 81,
                    [&sem](const SocketException& err) {
                        if (err) {
                            LOG_E << err.what();
                        }
                        else {
                            LOG_I << err.what();
                        }
                        sem.Post();
                    },
                    100);
            });
        }

        for (int i = 0; i < TEST_TIME; i++) {
            sem.Wait();
        }
    }

    LOG_D << "end ##########################################";
    sleep(1);

    return 0;
}
