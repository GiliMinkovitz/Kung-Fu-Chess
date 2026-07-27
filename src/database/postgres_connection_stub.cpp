#include "database/postgres_connection.h"

namespace kfc {

PostgresConnection::PostgresConnection(Settings settings)
    : settings_(std::move(settings)) {}

PostgresConnection::~PostgresConnection() = default;

bool PostgresConnection::open() {
    return false;
}

bool PostgresConnection::initialize_schema() {
    return false;
}

sqlite3* PostgresConnection::connection() {
    return nullptr;
}

PGconn* PostgresConnection::native_connection() const noexcept {
    return nullptr;
}

}  // namespace kfc
