#pragma once

#include <string>

namespace kfc::app {

enum class DatabaseBackend {
    SQLite,
    PostgreSQL,
};

struct DatabaseConfig {
    DatabaseBackend backend = DatabaseBackend::SQLite;
    std::string path = "kfc.db";
    std::string host = "localhost";
    int port = 5432;
    std::string database = "kfc";
    std::string username;
    std::string password;
};

[[nodiscard]] const char* database_backend_name(DatabaseBackend backend);

}  // namespace kfc::app
