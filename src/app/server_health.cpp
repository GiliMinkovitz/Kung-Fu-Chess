#include "app/server_health.h"

#include "app/database_config.h"
#include "app/i_runtime_store.h"
#include "database/i_database_connection.h"

namespace kfc::app {

HealthStatus make_health_status(const bool server_running, const DatabaseBackend backend,
                                const IDatabaseConnection& database, const RedisConfig& redis_config,
                                const IRuntimeStore& runtime_store) {
    return HealthStatus{
        server_running,
        database.is_connected(),
        database_backend_name(backend),
        redis_config.enabled,
        redis_config.enabled && runtime_store.is_available(),
    };
}

}  // namespace kfc::app
