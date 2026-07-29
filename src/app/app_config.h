#pragma once

#include "app/database_config.h"
#include "app/logging_config.h"
#include "app/matchmaking_config.h"
#include "app/redis_config.h"
#include "app/server_config.h"

namespace kfc::app {

struct AppConfig {
    ServerConfig server;
    DatabaseConfig database;
    RedisConfig redis;
    MatchmakingConfig matchmaking;
    LoggingConfig logging;
};

[[nodiscard]] AppConfig make_default_config();

}  // namespace kfc::app
