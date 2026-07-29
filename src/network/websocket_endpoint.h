#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace kfc {

struct WebSocketEndpoint {
    std::string host;
    std::uint16_t port;
};

[[nodiscard]] std::optional<WebSocketEndpoint> parse_websocket_endpoint(std::string_view endpoint);

}  // namespace kfc
