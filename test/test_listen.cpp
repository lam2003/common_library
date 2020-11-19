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

std::vector<Socket::Ptr> vec;
int main()
{
    
    Semaphore sem;
    //设置日志
    Logger::Instance().AddChannel(std::make_shared<ConsoleChannel>());
    Logger::Instance().SetWriter(
        std::make_shared<AsyncLogWriter>(Logger::Instance()));

    TimeTicker();

    EventPoller::Ptr poller = EventPollerPool::Instance().GetPoller();
    Socket::Ptr      socket = std::make_shared<Socket>(poller, true);

    socket->SetOnAccept([](Socket::Ptr& socket) {
        LOG_W << socket->GetPeerIP() << ":" << socket->GetPeerPort()
              << " connected";
        vec.emplace_back(socket);
    });
    
    socket->Listen(11111, false);
    sem.Wait();
    return 0;
}
