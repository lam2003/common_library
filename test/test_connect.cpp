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

#define CONNECTION_NUM 5

/**
 * socket工具库测试
 * @return
 */

Socket::Ptr      g_socket[CONNECTION_NUM];
EventPoller::Ptr g_poller        = EventPoller::Create();
char             g_buf[128 * 1024] = {1};

void             signal_handler(int signo)
{
    if (signo == SIGINT || signo == SIGTERM) {
        g_poller->Shutdown();
    }
}

void connect(int i)
{
    g_socket[i]->Connect(
        "localhost", 11111,
        [i](const SocketException& err) {
            if (err) {
                LOG_E << err.what();
                connect(i);
            }
            else {
                g_socket[i]->Send(g_buf, sizeof(g_buf));
            }
        },
        10);
}

int main()
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    Logger::Instance().AddChannel(std::make_shared<ConsoleChannel>());
    Logger::Instance().SetWriter(
        std::make_shared<AsyncLogWriter>(Logger::Instance()));

    for (int i = 0; i < CONNECTION_NUM; i++) {
        g_socket[i] = std::make_shared<Socket>(g_poller);
        g_socket[i]->SetOnError([i](const SocketException& e) {
            connect(i);
        });
        g_socket[i]->SetOnRead(
            [i](const Buffer::Ptr& buf, sockaddr_storage* addr, socklen_t len) {
                g_socket[i]->Close();
                connect(i);
            });
        g_socket[i]->SetOnFlushed([i]() {
            g_socket[i]->Close();
            connect(i);
            return true;
        });
    }

    for (int i = 0; i < CONNECTION_NUM; i++) {
        connect(i);
    }

    g_poller->RunLoop();
    LOG_D << "##################################################### end";
    return 0;
}
