#include <net/dns_cache.h>
#include <utils/logger.h>
#include <utils/utils.h>
#include <utils/uv_error.h>

#include <netdb.h>

namespace common_library {

INSTANCE_IMPL(DNSCache);

DNSCache::DNSCache() {}

bool DNSCache::Parse(const char* host, sockaddr_storage& addr, int expire_sec)
{
    DNSItem item;
    if (!get_cache_domain_ip(host, item, expire_sec)) {
        // 本地缓存中没找到，到系统缓存去找
        if (!get_system_domain_ip(host, item)) {
            return false;
        }
    }

    addr = item.addr;
    return true;
}

bool DNSCache::get_cache_domain_ip(const char* host,
                                   DNSItem&    item,
                                   int         expire_sec)
{
    bool exist = false;
    {
        auto it = dns_map_.find(host);
        if (it != dns_map_.end()) {
            item  = it->second;
            exist = true;
        }
    }

    if (exist && item.create_time + expire_sec < get_current_seconds()) {
        // timeout
        dns_map_.erase(host);
        return false;
    }

    return exist;
}

bool DNSCache::get_system_domain_ip(const char* host, DNSItem& item)
{
    addrinfo* answer = nullptr;

    int ret = -1;
    do {
        ret = getaddrinfo(host, nullptr, nullptr, &answer);
    } while (ret != 0 && get_uv_error() == EINTR);

    if (ret != 0 || !answer) {
        LOG_W << "dns failed. host=" << host;
        return false;
    }

    item.addr        = *(reinterpret_cast<sockaddr_storage*>(answer->ai_addr));
    item.create_time = get_current_seconds();
    {
        dns_map_[host] = item;
    }

    freeaddrinfo(answer);

    return true;
}

}  // namespace common_library