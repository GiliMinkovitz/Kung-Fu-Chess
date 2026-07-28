#include "app/config_loader.h"

#include <cstdlib>
#include <optional>
#include <string_view>

namespace {

std::optional<const char*> read_environment(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return std::nullopt;
    }
    return value;
}

std::optional<unsigned long> parse_unsigned(std::string_view value) {
    if (value.empty()) {
        return std::nullopt;
    }

    unsigned long result = 0;
    for (const char c : value) {
        if (c < '0' || c > '9') {
            return std::nullopt;
        }
        const unsigned long next = result * 10UL + static_cast<unsigned long>(c - '0');
        if (next < result) {
            return std::nullopt;
        }
        result = next;
    }
    return result;
}

void override_server_config(kfc::app::ServerConfig& server) {
    if (const auto port_value = read_environment("KFC_PORT")) {
        if (const auto port = parse_unsigned(*port_value)) {
            if (*port >= 1 && *port <= 65535) {
                server.port = static_cast<unsigned short>(*port);
            }
        }
    }

    if (const auto bind_value = read_environment("KFC_BIND_ADDRESS")) {
        server.bind_address = *bind_value;
    }

    if (const auto max_clients_value = read_environment("KFC_MAX_CLIENTS")) {
        if (const auto max_clients = parse_unsigned(*max_clients_value)) {
            if (*max_clients > 0) {
                server.max_clients = static_cast<std::size_t>(*max_clients);
            }
        }
    }
}

}  // namespace

namespace kfc::app {

AppConfig load_config_from_environment() {
    AppConfig config = make_default_config();
    override_server_config(config.server);
    return config;
}

}  // namespace kfc::app
