#include "app/database_config.h"

#include <doctest/doctest.h>

#include <string>

TEST_CASE("DatabaseConfigTest - DatabaseBackendName") {
    CHECK_EQ(std::string(kfc::app::database_backend_name(kfc::app::DatabaseBackend::SQLite)),
             "SQLite");
    CHECK_EQ(std::string(kfc::app::database_backend_name(kfc::app::DatabaseBackend::PostgreSQL)),
             "PostgreSQL");
}
