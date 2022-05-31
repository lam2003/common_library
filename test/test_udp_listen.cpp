#include "net/dns_cache.h"
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

int main(int argc, char** argv)
{
    //设置日志
    Logger::Instance().AddChannel(std::make_shared<ConsoleChannel>());
    Logger::Instance().SetWriter(
        std::make_shared<AsyncLogWriter>(Logger::Instance()));

    g_poller = EventPoller::Create();
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    Socket::Ptr sock = Socket::Create(g_poller);

    sock->SetOnRead([sock](const Buffer::Ptr& buf, sockaddr_storage* addr,
                           socklen_t addr_len) {
        sock->Send(buf.get()->Data(), buf.get()->Size(), addr, addr_len);
    });

    sockaddr_storage ss;
    DNSCache::Instance().Parse(argv[1], ss);
    ((sockaddr_in6*)reinterpret_cast<sockaddr_in6*>(&ss))->sin6_port =
        htons(11111);
    sock->SetOnFlushed([&sock, &ss]() {
        sock->Send("helloworld", 10, &ss, sizeof(ss));
        return true;
    });

    // std::thread([&sock,&ss](){
    //     while(1){
    //         sock->
    //     };
    // }).detach();
    sock->Listen(SOCK_UDP, 11111, true, "ens160");

    sock->Send("helloworld", 10, &ss, sizeof(ss));

    LOG_D << "\n"
          << sock->GetLocalIP() << "\n"
          << sock->GetLocalPort() << std::endl;

    g_poller->RunLoop();
// fe80::250:56ff:fe85:c98d 
    LOG_D << "########################################################### END";

    return 0;
}
