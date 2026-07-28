#pragma once

#include "app/database_config.h"
#include "app/health_status.h"

namespace kfc {

class IDatabaseConnection;

}  // namespace kfc

namespace kfc::app {

[[nodiscard]] HealthStatus make_health_status(bool server_running, DatabaseBackend backend,
                                              const IDatabaseConnection& database);

}  // namespace kfc::app
