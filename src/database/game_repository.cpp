#include "database/game_repository.h"

#include <sqlite3.h>

namespace kfc {

GameRepository::GameRepository(SqliteDatabase& database) : database_(database) {}

std::optional<int> GameRepository::create_game(int white_player_id, int black_player_id) {
    sqlite3* db = database_.connection();
    if (db == nullptr) {
        return std::nullopt;
    }

    sqlite3_stmt* stmt = nullptr;
    constexpr const char* sql =
        "INSERT INTO games (white_player_id, black_player_id, status, created_at) "
        "VALUES (?, ?, 'active', datetime('now'));";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, white_player_id);
    sqlite3_bind_int(stmt, 2, black_player_id);

    const bool inserted = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    if (!inserted) {
        return std::nullopt;
    }

    return static_cast<int>(sqlite3_last_insert_rowid(db));
}

bool GameRepository::finish_game(int game_id, int winner_player_id) {
    sqlite3* db = database_.connection();
    if (db == nullptr) {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    constexpr const char* sql =
        "UPDATE games SET status = 'finished', winner_id = ? WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, winner_player_id);
    sqlite3_bind_int(stmt, 2, game_id);

    const bool updated = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return updated;
}

}  // namespace kfc
