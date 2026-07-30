#pragma once

#include "app/server_config.h"

#include <string>
#include <string_view>

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

[[nodiscard]] inline std::string resolve_matchmaker_health_endpoint(const ServerConfig& server) {
    constexpr std::string_view prefix = "http://";
    const std::string& endpoint = server.matchmaker_endpoint;
    if (endpoint.size() >= prefix.size() && endpoint.compare(0, prefix.size(), prefix) == 0) {
        const std::string_view remainder = std::string_view(endpoint).substr(prefix.size());
        const std::size_t slash = remainder.find('/');
        const std::string_view host_port =
            slash == std::string_view::npos ? remainder : remainder.substr(0, slash);
        const std::size_t colon = host_port.find(':');
        const std::string host = colon == std::string_view::npos
                                     ? std::string(host_port)
                                     : std::string(host_port.substr(0, colon));
        return "http://" + host + ":" + std::to_string(server.matchmaker_health_port);
    }
    return "http://" + server.bind_address + ":" + std::to_string(server.matchmaker_health_port);
}

}  // namespace kfc::app
