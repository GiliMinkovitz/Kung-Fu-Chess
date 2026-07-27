#pragma once

#include <cstddef>
#include <string>

namespace kfc::app {

struct ServerConfig {
    static constexpr std::size_t kDefaultMaxClients = 8;

    unsigned short port = 8765;
    std::string bind_address = "127.0.0.1";
    std::size_t max_clients = kDefaultMaxClients;
};

}  // namespace kfc::app
