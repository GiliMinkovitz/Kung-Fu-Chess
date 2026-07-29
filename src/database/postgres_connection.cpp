#include "database/postgres_connection.h"

#include <sstream>

namespace kfc {

namespace {

constexpr const char* kPlayersTableSql = R"(
CREATE TABLE IF NOT EXISTS players (
    id SERIAL PRIMARY KEY,
    username TEXT NOT NULL UNIQUE,
    password_hash TEXT,
    rating INTEGER NOT NULL DEFAULT 1000,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
)";

constexpr const char* kGamesTableSql = R"(
CREATE TABLE IF NOT EXISTS games (
    id SERIAL PRIMARY KEY,
    white_player_id INTEGER NOT NULL REFERENCES players(id),
    black_player_id INTEGER NOT NULL REFERENCES players(id),
    winner_id INTEGER REFERENCES players(id),
    status TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    finished_at TIMESTAMPTZ
);
)";

constexpr const char* kGamesIndexesSql = R"(
CREATE INDEX IF NOT EXISTS idx_games_white_player_id ON games(white_player_id);
CREATE INDEX IF NOT EXISTS idx_games_black_player_id ON games(black_player_id);
CREATE INDEX IF NOT EXISTS idx_games_status ON games(status);
)";

struct PgResultGuard {
    PGresult* result = nullptr;

    ~PgResultGuard() {
        if (result != nullptr) {
            PQclear(result);
        }
    }
};

bool exec_sql(PGconn* connection, const char* sql) {
    if (connection == nullptr || PQstatus(connection) != CONNECTION_OK) {
        return false;
    }

    PgResultGuard guard;
    guard.result = PQexec(connection, sql);
    if (guard.result == nullptr) {
        return false;
    }

    return PQresultStatus(guard.result) == PGRES_COMMAND_OK;
}

}  // namespace

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
    if (connection_ == nullptr || PQstatus(connection_) != CONNECTION_OK) {
        return false;
    }

    return exec_sql(connection_, kPlayersTableSql) && exec_sql(connection_, kGamesTableSql) &&
           exec_sql(connection_, kGamesIndexesSql);
}

bool PostgresConnection::is_connected() const {
    return connection_ != nullptr && PQstatus(connection_) == CONNECTION_OK;
}

sqlite3* PostgresConnection::connection() {
    return nullptr;
}

PGconn* PostgresConnection::native_connection() const noexcept {
    return connection_;
}

}  // namespace kfc
