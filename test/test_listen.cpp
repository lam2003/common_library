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

#define TEST_TIME 10
std::map<std::string, Socket::Ptr> g_sockets;
EventPoller::Ptr                   g_poller;

void signal_handler(int signo)
{
    if (signo == SIGINT || signo == SIGTERM) {
        g_poller->Shutdown();
    }
}

int main()
{
    //设置日志
    Logger::Instance().AddChannel(std::make_shared<ConsoleChannel>());
    Logger::Instance().SetWriter(
        std::make_shared<AsyncLogWriter>(Logger::Instance()));

    g_poller = EventPoller::Create();
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    Socket::Ptr sock = Socket::Create(g_poller);

    sock->SetOnError([](const SocketException& err) {});
    sock->SetOnAccept([](Socket::Ptr& cli) {
        g_sockets.insert(std::make_pair(cli->GetIdentifier(), cli));
        std::weak_ptr<Socket> weak_cli = cli;

        cli->SetOnRead(
            [](const Buffer::Ptr& buf, sockaddr_storage* addr, socklen_t len) {

            });
        cli->SetOnError([weak_cli](const SocketException& err) {
            Socket::Ptr strong_cli = weak_cli.lock();
            if (strong_cli) {
                g_sockets.erase(strong_cli->GetIdentifier());
            }
        });
    });

    sock->Listen(SOCK_TCP, 11111, true, "ens160");
    LOG_D << "\n"
          << sock->GetLocalIP() << "\n"
          << sock->GetLocalPort() << std::endl;

    g_poller->RunLoop();

    LOG_D << "########################################################### END";

    return 0;
}
