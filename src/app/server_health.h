#pragma once

#include "app/database_config.h"
#include "app/health_status.h"
#include "app/redis_config.h"

namespace kfc {

class IDatabaseConnection;
class IRuntimeStore;

}  // namespace kfc

namespace kfc::app {

[[nodiscard]] HealthStatus make_health_status(bool server_running, DatabaseBackend backend,
                                              const IDatabaseConnection& database,
                                              const RedisConfig& redis_config,
                                              const IRuntimeStore& runtime_store);

}  // namespace kfc::app
