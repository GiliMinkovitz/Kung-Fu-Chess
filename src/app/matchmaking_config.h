#pragma once

#include <chrono>

namespace kfc::app {

struct MatchmakingConfig {
    int max_rating_difference = 100;
    std::chrono::seconds queue_timeout{60};
};

}  // namespace kfc::app
