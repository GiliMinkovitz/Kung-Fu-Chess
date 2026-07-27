#include "database/postgres_connection.h"

#include <sstream>

namespace kfc {

PostgresConnection::PostgresConnection(Settings settings)
    : settings_(std::move(settings)) {}

PostgresConnection::~PostgresConnection() {
    if (connection_ != nullptr) {
        PQfinish(connection_);
        connection_ = nullptr;
    }
}

bool PostgresConnection::open() {
    if (connection_ != nullptr) {
        return PQstatus(connection_) == CONNECTION_OK;
    }

    std::ostringstream conninfo;
    conninfo << "host=" << settings_.host << " port=" << settings_.port
             << " dbname=" << settings_.database;

    if (!settings_.username.empty()) {
        conninfo << " user=" << settings_.username;
    }
    if (!settings_.password.empty()) {
        conninfo << " password=" << settings_.password;
    }

    connection_ = PQconnectdb(conninfo.str().c_str());
    return connection_ != nullptr && PQstatus(connection_) == CONNECTION_OK;
}

bool PostgresConnection::initialize_schema() {
    return connection_ != nullptr && PQstatus(connection_) == CONNECTION_OK;
}

sqlite3* PostgresConnection::connection() {
    return nullptr;
}

PGconn* PostgresConnection::native_connection() const noexcept {
    return connection_;
}

}  // namespace kfc
