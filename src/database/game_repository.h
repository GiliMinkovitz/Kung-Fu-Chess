#pragma once

#include "database/i_game_repository.h"
#include "database/sqlite_database.h"

#include <optional>

namespace kfc {

class GameRepository : public IGameRepository {
public:
    explicit GameRepository(SqliteDatabase& database);

    [[nodiscard]] std::optional<int> create_game(int white_player_id,
                                                 int black_player_id) override;
    [[nodiscard]] bool finish_game(int game_id, int winner_player_id) override;
    [[nodiscard]] bool finish_game_without_winner(int game_id) override;

private:
    SqliteDatabase& database_;
};

}  // namespace kfc
