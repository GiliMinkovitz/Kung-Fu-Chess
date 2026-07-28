#include "app/database_config.h"

namespace kfc::app {

const char* database_backend_name(DatabaseBackend backend) {
    switch (backend) {
        case DatabaseBackend::SQLite:
            return "SQLite";
        case DatabaseBackend::PostgreSQL:
            return "PostgreSQL";
    }

    return "Unknown";
}

}  // namespace kfc::app
