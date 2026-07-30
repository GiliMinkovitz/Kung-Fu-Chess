#pragma once

#include "app/redis_config.h"

#include <chrono>
#include <string>

namespace kfc {

class IDatabaseConnection;
class IRuntimeStore;

}  // namespace kfc

namespace kfc::app::observability {

[[nodiscard]] bool is_redis_ready(const RedisConfig& redis_config, const IRuntimeStore& runtime_store);
[[nodiscard]] bool is_database_ready(const IDatabaseConnection& database);
[[nodiscard]] bool is_http_endpoint_reachable(const std::string& endpoint,
                                              std::chrono::milliseconds timeout);

[[nodiscard]] bool check_gateway_ready(const IDatabaseConnection& database,
                                       const RedisConfig& redis_config,
                                       const IRuntimeStore& runtime_store,
                                       const std::string& matchmaker_health_endpoint);

[[nodiscard]] bool check_matchmaker_ready(const IDatabaseConnection& database,
                                        const RedisConfig& redis_config,
                                        const IRuntimeStore& runtime_store);

[[nodiscard]] bool check_game_server_ready(const IDatabaseConnection& database,
                                           const RedisConfig& redis_config,
                                           const IRuntimeStore& runtime_store,
                                           bool allocation_api_active);

}  // namespace kfc::app::observability
