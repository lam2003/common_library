#ifndef COMMON_LIBRARY_DNS_CACHE_H
#define COMMON_LIBRARY_DNS_CACHE_H

#include <utils/mutex_wrapper.h>

#include <unordered_map>

#include <sys/socket.h>

namespace common_library {

class DNSCache final {
  public:
    static DNSCache& Instance();

    ~DNSCache() = default;

    bool Parse(const char* host, sockaddr_storage& addr, int expire_sec = 60);

  private:
    struct DNSItem
    {
        sockaddr_storage addr;
        // unix seconds
        uint64_t create_time;
    };

    DNSCache(bool enable_mutex = true);

    bool get_cache_domain_ip(const char* host, DNSItem& item, int expire_sec);

    bool get_system_domain_ip(const char* host, DNSItem& item);

  private:
    MutexWrapper<std::mutex>                 mux_;
    std::unordered_map<std::string, DNSItem> dns_map_;
};

}  // namespace common_library

#endif