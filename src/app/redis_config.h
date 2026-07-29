#pragma once

#include <chrono>
#include <string>

namespace kfc::app {

struct RedisConfig {
    bool enabled = false;
    std::string host = "127.0.0.1";
    int port = 6379;
    std::string password;
    int database = 0;
    std::chrono::seconds heartbeat_interval{1};
};

}  // namespace kfc::app
