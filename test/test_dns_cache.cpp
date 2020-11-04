#include "net/dns_cache.h"
#include "thread/worker.h"
#include "utils/logger.h"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;
using namespace common_library;

/**
 * dns缓存测试
 * @return
 */

std::atomic<uint32_t> run_time{1};
Semaphore             sem_;

int main()
{
    //设置日志
    Logger::Instance().AddChannel(std::make_shared<ConsoleChannel>());
    Logger::Instance().SetWriter(
        std::make_shared<AsyncLogWriter>(Logger::Instance()));

    TimeTicker();
    for (int i = 0; i < 1000000; i++) {
        common_library::WorkerPool::Instance().GetWorker()->Async([]() {
            sockaddr_storage addr;
            bool ok = DNSCache::Instance().Parse("linmin.xyz", addr, 10);
            if (!ok) {
                return;
            }
            int         family = ((sockaddr_in*)&addr)->sin_family;
            char        buf[INET6_ADDRSTRLEN + 1] = {0};
            std::string addr_str;

            if (family == AF_INET6) {
                addr_str             = inet_ntop(AF_INET6,
                                     (void*)&((sockaddr_in6*)&addr)->sin6_addr,
                                     buf, INET6_ADDRSTRLEN);
                buf[INET_ADDRSTRLEN] = '\0';
                addr_str             = buf;
                // LOG_I << "is ipv6 addr:" << addr_str;
            }
            else if (family == AF_INET) {
                addr_str = inet_ntop(AF_INET, &((sockaddr_in*)&addr)->sin_addr,
                                     buf, sizeof(buf));
                // LOG_I << "is ipv4 addr:" << addr_str;
            }

            if (run_time.fetch_add(1, std::memory_order_acquire) == 1000000) {
                sem_.Post(1);
            }
        });
    }
    sem_.Wait();
    return 0;
}
