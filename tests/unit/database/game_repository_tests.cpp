#include "database/game_repository.h"
#include "database/sqlite_database.h"

#include <doctest/doctest.h>

#include <fstream>
#include <sqlite3.h>
#include <string>

namespace {

kfc::SqliteDatabase& shared_db() {
    static kfc::SqliteDatabase db(":memory:");
    static bool initialized = false;
    if (!initialized) {
        REQUIRE(db.open());
        REQUIRE(db.initialize_schema());
        initialized = true;
    }
    return db;
}

std::pair<int, int> create_players(kfc::SqliteDatabase& db) {
    static int next_game = 0;
    const std::string white_name = "white_player_" + std::to_string(next_game);
    const std::string black_name = "black_player_" + std::to_string(next_game);
    ++next_game;

    sqlite3* connection = db.connection();
    REQUIRE(connection != nullptr);

    sqlite3_stmt* stmt = nullptr;
    REQUIRE(sqlite3_prepare_v2(connection, "INSERT INTO players (username, rating) VALUES (?, ?);",
                               -1, &stmt, nullptr) == SQLITE_OK);
    sqlite3_bind_text(stmt, 1, white_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, 1000);
    REQUIRE(sqlite3_step(stmt) == SQLITE_DONE);
    const int white_id = static_cast<int>(sqlite3_last_insert_rowid(connection));
    sqlite3_finalize(stmt);

    REQUIRE(sqlite3_prepare_v2(connection, "INSERT INTO players (username, rating) VALUES (?, ?);",
                               -1, &stmt, nullptr) == SQLITE_OK);
    sqlite3_bind_text(stmt, 1, black_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, 1000);
    REQUIRE(sqlite3_step(stmt) == SQLITE_DONE);
    const int black_id = static_cast<int>(sqlite3_last_insert_rowid(connection));
    sqlite3_finalize(stmt);

    return {white_id, black_id};
}

}  // namespace

TEST_CASE("GameRepositoryTest - CreatesActiveGame") {
    kfc::GameRepository repo{shared_db()};
    const auto [white_id, black_id] = create_players(shared_db());
    const auto game_id = repo.create_game(white_id, black_id);
    REQUIRE(game_id.has_value());
    CHECK(*game_id > 0);
}

TEST_CASE("GameRepositoryTest - FinishesGameWithWinner") {
    kfc::GameRepository repo{shared_db()};
    const auto [white_id, black_id] = create_players(shared_db());
    const auto game_id = repo.create_game(white_id, black_id);
    REQUIRE(game_id.has_value());

    CHECK(repo.finish_game(*game_id, white_id));

    sqlite3* connection = shared_db().connection();
    sqlite3_stmt* stmt = nullptr;
    REQUIRE(sqlite3_prepare_v2(connection,
                               "SELECT status, winner_id FROM games WHERE id = ? LIMIT 1;", -1,
                               &stmt, nullptr) == SQLITE_OK);
    sqlite3_bind_int(stmt, 1, *game_id);
    REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
    CHECK_EQ(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))),
             "finished");
    CHECK_EQ(sqlite3_column_int(stmt, 1), white_id);
    sqlite3_finalize(stmt);
}

TEST_CASE("GameRepositoryTest - ReturnsNulloptWithoutDatabaseConnection") {
    kfc::SqliteDatabase db(":memory:");
    kfc::GameRepository repo{db};

    CHECK_FALSE(repo.create_game(1, 2).has_value());
    CHECK_FALSE(repo.finish_game(1, 1));
}

TEST_CASE("GameRepositoryTest - RejectsWriteWhenDatabaseIsReadOnly") {
    const char* path = "kfc_readonly_game_repo_test.db";
    {
        kfc::SqliteDatabase setup(path);
        REQUIRE(setup.open());
        REQUIRE(setup.initialize_schema());
    }

    kfc::SqliteDatabase db(path);
    REQUIRE(db.open());
    kfc::GameRepository repo{db};

    sqlite3* connection = db.connection();
    REQUIRE(connection != nullptr);
    REQUIRE(sqlite3_exec(connection, "PRAGMA query_only = ON;", nullptr, nullptr, nullptr) ==
            SQLITE_OK);

    CHECK_FALSE(repo.create_game(1, 2).has_value());
    CHECK_FALSE(repo.finish_game(1, 1));
    std::remove(path);
}

TEST_CASE("GameRepositoryTest - HandlesCorruptDatabaseFile") {
    const char* path = "kfc_corrupt_game_repo_test.db";
    {
        std::ofstream out(path, std::ios::binary);
        out << "not-a-sqlite-database";
    }

    kfc::SqliteDatabase db(path);
    REQUIRE(db.open());
    kfc::GameRepository repo{db};

    CHECK_FALSE(repo.create_game(1, 2).has_value());
    CHECK_FALSE(repo.finish_game(1, 1));
    std::remove(path);
}
