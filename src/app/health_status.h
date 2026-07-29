#pragma once

#include <string>

namespace kfc::app {

struct HealthStatus {
    bool server_running = false;
    bool database_connected = false;
    std::string database_backend;
    bool redis_enabled = false;
    bool redis_connected = false;
};

}  // namespace kfc::app
