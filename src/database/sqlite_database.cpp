#include "sqlite_database.h"

namespace kfc {

namespace {

constexpr const char* kPlayersTableSql = R"(
CREATE TABLE IF NOT EXISTS players (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE
);
)";

constexpr const char* kGamesTableSql = R"(
CREATE TABLE IF NOT EXISTS games (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    white_player_id INTEGER,
    black_player_id INTEGER,
    winner_id INTEGER,
    status TEXT NOT NULL,
    created_at TEXT NOT NULL
);
)";

bool exec_sql(sqlite3* db, const char* sql) {
    char* err_msg = nullptr;
    const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

}  // namespace

SqliteDatabase::SqliteDatabase(const std::string& path) : path_(path) {}

SqliteDatabase::~SqliteDatabase() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool SqliteDatabase::open() {
    if (db_ != nullptr) {
        return true;
    }

    const int rc = sqlite3_open(path_.c_str(), &db_);
    return rc == SQLITE_OK;
}

bool SqliteDatabase::initialize_schema() {
    if (db_ == nullptr) {
        return false;
    }

    return exec_sql(db_, kPlayersTableSql) && exec_sql(db_, kGamesTableSql);
}

sqlite3* SqliteDatabase::connection() {
    return db_;
}

}  // namespace kfc
