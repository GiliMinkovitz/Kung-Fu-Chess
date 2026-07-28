#pragma once

#include "database/i_game_repository.h"
#include "database/postgres_connection.h"

#include <optional>

namespace kfc {

// Requires games table created by PostgresConnection::initialize_schema().

class PostgresGameRepository final : public IGameRepository {
public:
    explicit PostgresGameRepository(PostgresConnection& database);

    [[nodiscard]] std::optional<int> create_game(int white_player_id,
                                                 int black_player_id) override;
    [[nodiscard]] bool finish_game(int game_id, int winner_player_id) override;
    [[nodiscard]] bool finish_game_without_winner(int game_id) override;

private:
    PostgresConnection& database_;
};

}  // namespace kfc
