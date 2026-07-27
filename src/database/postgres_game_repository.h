#pragma once

#include "database/i_game_repository.h"
#include "database/postgres_connection.h"

#include <optional>

namespace kfc {

// Expected PostgreSQL schema (apply via external migration tooling; not created here):
//
// CREATE TABLE games (
//     id SERIAL PRIMARY KEY,
//     white_player_id INTEGER,
//     black_player_id INTEGER,
//     winner_id INTEGER,
//     status TEXT NOT NULL,
//     created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
// );

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
