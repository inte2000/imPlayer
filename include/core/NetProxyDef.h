#ifndef NET_PROXY_DEFINE_H
#define NET_PROXY_DEFINE_H

#include <string>
#include <memory>
#include <vector>

enum class NetProxyType
{
    none = 0,
    http,
    https,
    socks4,
    socks5
};

struct NetProxy
{
    NetProxyType type;
    std::string serverName;
    int serverPort;
    std::string userName;
    std::string password;
    
};

#endif //NET_PROXY_DEFINE_H
