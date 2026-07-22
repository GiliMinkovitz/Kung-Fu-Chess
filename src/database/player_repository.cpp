#include "database/player_repository.h"

#include <sqlite3.h>

namespace kfc {

PlayerRepository::PlayerRepository(SqliteDatabase& database) : database_(database) {}

std::optional<Player> PlayerRepository::find_by_username(const std::string& username) {
    sqlite3* db = database_.connection();
    if (db == nullptr) {
        return std::nullopt;
    }

    sqlite3_stmt* stmt = nullptr;
    constexpr const char* sql =
        "SELECT id, username, rating FROM players WHERE username = ? LIMIT 1;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<Player> player;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const int id = sqlite3_column_int(stmt, 0);
        const unsigned char* name = sqlite3_column_text(stmt, 1);
        const int rating = sqlite3_column_int(stmt, 2);
        if (name != nullptr) {
            player = Player(id, reinterpret_cast<const char*>(name), rating);
        }
    }

    sqlite3_finalize(stmt);
    return player;
}

std::optional<Player> PlayerRepository::create_player(const std::string& username, int rating) {
    sqlite3* db = database_.connection();
    if (db == nullptr) {
        return std::nullopt;
    }

    sqlite3_stmt* stmt = nullptr;
    constexpr const char* sql = "INSERT INTO players (username, rating) VALUES (?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, rating);

    const bool inserted = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    if (!inserted) {
        return std::nullopt;
    }

    const int id = static_cast<int>(sqlite3_last_insert_rowid(db));
    return Player(id, username, rating);
}

bool PlayerRepository::update_rating(int player_id, int rating) {
    sqlite3* db = database_.connection();
    if (db == nullptr) {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    constexpr const char* sql = "UPDATE players SET rating = ? WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, rating);
    sqlite3_bind_int(stmt, 2, player_id);

    const bool updated = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return updated;
}

}  // namespace kfc
