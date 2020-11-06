#include "net/socket.h"
#include "net/socket_utils.h"
#include "poller/event_poller.h"
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

/**
 * socket工具库测试
 * @return
 */

Semaphore sem;

int main()
{
    //设置日志
    Logger::Instance().AddChannel(std::make_shared<ConsoleChannel>());
    Logger::Instance().SetWriter(
        std::make_shared<AsyncLogWriter>(Logger::Instance()));

    TimeTicker();

    EventPoller::Ptr poller = EventPollerPool::Instance().GetPoller();
    Socket::Ptr      socket = std::make_shared<Socket>(poller, false);
    socket->Connect("ipv6.linmin.xyz", 80, [](const SocketException& err) {
        if (err) {
            LOG_E << err.what();
        }
        else {
            LOG_I << err.what();
        }
        sem.Post();
    });

    sem.Wait();

    socket->Connect("baidu.com", 80, [](const SocketException& err) {
        if (err) {
            LOG_E << err.what();
        }
        else {
            LOG_I << err.what();
        }
        sem.Post();
    });

    sem.Wait();

    return 0;
}
