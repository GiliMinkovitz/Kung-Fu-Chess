#pragma once

#include <cstddef>
#include <string>

namespace kfc::app {

struct ServerConfig {
    static constexpr std::size_t kDefaultMaxClients = 8;

    unsigned short port = 8765;
    unsigned short game_port = 8766;
    unsigned short health_port = 8080;
    unsigned short game_health_port = 8081;
    unsigned short game_internal_port = 8767;
    unsigned short matchmaker_port = 8770;
    unsigned short matchmaker_health_port = 8772;
    unsigned short gateway_internal_port = 8771;
    std::string bind_address = "127.0.0.1";
    std::string server_id = "local";
    std::string gateway_server_id = "gateway-local";
    std::string region = "local";
    std::string endpoint;
    std::string allocation_endpoint;
    std::string matchmaker_endpoint = "http://127.0.0.1:8770";
    std::string gateway_notification_endpoint = "http://127.0.0.1:8771";
    std::size_t max_clients = kDefaultMaxClients;
    std::size_t game_max_clients = kDefaultMaxClients;
};

}  // namespace kfc::app
