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

#define TEST_TIME 10
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



    EventPoller::Ptr poller = EventPollerPool::Instance().GetPoller();
    Socket::Ptr      socket;
    socket = std::make_shared<Socket>(poller, false);

    socket->Connect("linmin.xyz", 11111,
                    [](const SocketException& err) { LOG_W << err.what(); });
    char buf[128 * 1024] = {8};
   
    socket->SetOnFlushed([&sem]() {
        LOG_I << "write done########################### ";
        sem.Post();
        return true;
    });

    TimeTicker();
    int i = 1024;
    while (i--) {
        usleep(10);
        socket->Send(buf, sizeof(buf));
    }
    getchar();
    return 0;
}
