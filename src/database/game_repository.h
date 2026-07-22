#pragma once

#include "database/sqlite_database.h"

#include <optional>

namespace kfc {

class GameRepository {
public:
    explicit GameRepository(SqliteDatabase& database);

    [[nodiscard]] std::optional<int> create_game(int white_player_id, int black_player_id);
    [[nodiscard]] bool finish_game(int game_id, int winner_player_id);

private:
    SqliteDatabase& database_;
};

}  // namespace kfc
