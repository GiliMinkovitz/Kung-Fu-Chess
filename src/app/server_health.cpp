#include "app/server_health.h"

#include "app/database_config.h"
#include "database/i_database_connection.h"

namespace kfc::app {

HealthStatus make_health_status(const bool server_running, const DatabaseBackend backend,
                                const IDatabaseConnection& database) {
    return HealthStatus{
        server_running,
        database.is_connected(),
        database_backend_name(backend),
    };
}

}  // namespace kfc::app
