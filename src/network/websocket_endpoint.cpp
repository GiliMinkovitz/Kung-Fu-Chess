#include "network/websocket_endpoint.h"

namespace kfc {

namespace {

std::string_view strip_scheme(std::string_view endpoint) {
    if (endpoint.rfind("ws://", 0) == 0) {
        return endpoint.substr(5);
    }
    if (endpoint.rfind("wss://", 0) == 0) {
        return endpoint.substr(6);
    }
    return endpoint;
}

}  // namespace

std::optional<WebSocketEndpoint> parse_websocket_endpoint(std::string_view endpoint) {
    endpoint = strip_scheme(endpoint);
    if (endpoint.empty()) {
        return std::nullopt;
    }

    const std::size_t colon = endpoint.rfind(':');
    if (colon == std::string_view::npos || colon == 0 || colon + 1 >= endpoint.size()) {
        return std::nullopt;
    }

    const std::string host{endpoint.substr(0, colon)};
    const std::string_view port_text = endpoint.substr(colon + 1);

    std::uint32_t port_value = 0;
    for (const char ch : port_text) {
        if (ch < '0' || ch > '9') {
            return std::nullopt;
        }
        port_value = port_value * 10 + static_cast<std::uint32_t>(ch - '0');
        if (port_value > 65535) {
            return std::nullopt;
        }
    }
    if (port_value == 0) {
        return std::nullopt;
    }

    return WebSocketEndpoint{host, static_cast<std::uint16_t>(port_value)};
}

}  // namespace kfc
