#include "database/player_repository.h"

#include <sqlite3.h>

namespace kfc {

PlayerRepository::PlayerRepository(SqliteDatabase& database) : database_(database) {}

std::optional<Player> PlayerRepository::find_by_username(const std::string& username) const {
    if (const auto credentials = find_credentials_by_username(username)) {
        return Player(credentials->id, credentials->username, credentials->rating);
    }
    return std::nullopt;
}

std::optional<Player> PlayerRepository::find_by_id(int player_id) const {
    sqlite3* db = database_.connection();
    if (db == nullptr) {
        return std::nullopt;
    }

    sqlite3_stmt* stmt = nullptr;
    constexpr const char* sql =
        "SELECT id, username, rating FROM players WHERE id = ? LIMIT 1;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, player_id);

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

std::optional<PlayerCredentials> PlayerRepository::find_credentials_by_username(
    const std::string& username) const {
    sqlite3* db = database_.connection();
    if (db == nullptr) {
        return std::nullopt;
    }

    sqlite3_stmt* stmt = nullptr;
    constexpr const char* sql =
        "SELECT id, username, rating, password_hash FROM players WHERE username = ? LIMIT 1;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<PlayerCredentials> credentials;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const int id = sqlite3_column_int(stmt, 0);
        const unsigned char* name = sqlite3_column_text(stmt, 1);
        const int rating = sqlite3_column_int(stmt, 2);
        const unsigned char* password_hash = sqlite3_column_text(stmt, 3);
        if (name != nullptr) {
            credentials = PlayerCredentials{
                id,
                reinterpret_cast<const char*>(name),
                rating,
                password_hash != nullptr ? reinterpret_cast<const char*>(password_hash) : "",
            };
        }
    }

    sqlite3_finalize(stmt);
    return credentials;
}

std::optional<Player> PlayerRepository::create_player(const std::string& username, int rating,
                                                      const std::string& password_hash) {
    sqlite3* db = database_.connection();
    if (db == nullptr) {
        return std::nullopt;
    }

    sqlite3_stmt* stmt = nullptr;
    constexpr const char* sql =
        "INSERT INTO players (username, rating, password_hash) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, rating);
    if (password_hash.empty()) {
        sqlite3_bind_null(stmt, 3);
    } else {
        sqlite3_bind_text(stmt, 3, password_hash.c_str(), -1, SQLITE_TRANSIENT);
    }

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
