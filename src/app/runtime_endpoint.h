#pragma once

#include "app/server_config.h"

#include <string>

namespace kfc::app {

[[nodiscard]] inline std::string resolve_game_endpoint(const ServerConfig& server) {
    if (!server.endpoint.empty()) {
        return server.endpoint;
    }
    return "ws://" + server.bind_address + ":" + std::to_string(server.game_port);
}

[[nodiscard]] inline std::string resolve_allocation_endpoint(const ServerConfig& server) {
    if (!server.allocation_endpoint.empty()) {
        return server.allocation_endpoint;
    }
    return "http://" + server.bind_address + ":" + std::to_string(server.game_internal_port) +
           "/allocate";
}

}  // namespace kfc::app
