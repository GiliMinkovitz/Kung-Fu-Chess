#pragma once

#include <chrono>
#include <cstddef>
#include <string>

namespace kfc::app {

struct AllocationConfig {
    std::string internal_service_token;
    std::chrono::milliseconds allocation_timeout{1000};
    std::size_t allocation_retry_count = 3;
    std::chrono::seconds game_server_ttl{10};
};

}  // namespace kfc::app
