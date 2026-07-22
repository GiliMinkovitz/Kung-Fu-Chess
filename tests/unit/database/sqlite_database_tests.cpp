#include "database/sqlite_database.h"

#include <doctest/doctest.h>

#include <set>
#include <string>

namespace {

bool table_exists(sqlite3* db, const std::string& table_name) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = ? LIMIT 1;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, table_name.c_str(), -1, SQLITE_TRANSIENT);

    const bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return exists;
}

std::set<std::string> list_user_tables(sqlite3* db) {
    std::set<std::string> tables;
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT name FROM sqlite_master WHERE type = 'table' AND name NOT LIKE 'sqlite_%';";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return tables;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* name = sqlite3_column_text(stmt, 0);
        if (name != nullptr) {
            tables.insert(reinterpret_cast<const char*>(name));
        }
    }

    sqlite3_finalize(stmt);
    return tables;
}

}  // namespace

TEST_CASE("SqliteDatabaseTest - OpensInMemoryDatabase") {
    kfc::SqliteDatabase db(":memory:");
    CHECK(db.open());
    CHECK(db.connection() != nullptr);
}

TEST_CASE("SqliteDatabaseTest - InitializesSchema") {
    kfc::SqliteDatabase db(":memory:");
    REQUIRE(db.open());
    CHECK(db.initialize_schema());
}

TEST_CASE("SqliteDatabaseTest - CreatesExpectedTables") {
    kfc::SqliteDatabase db(":memory:");
    REQUIRE(db.open());
    REQUIRE(db.initialize_schema());

    sqlite3* connection = db.connection();
    REQUIRE(connection != nullptr);

    CHECK(table_exists(connection, "players"));
    CHECK(table_exists(connection, "games"));

    const std::set<std::string> tables = list_user_tables(connection);
    CHECK_EQ(tables.size(), 2);
    CHECK(tables.count("players") == 1);
    CHECK(tables.count("games") == 1);
}
