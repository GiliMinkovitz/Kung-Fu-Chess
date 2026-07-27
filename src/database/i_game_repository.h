#pragma once

#include <optional>

namespace kfc {

class IGameRepository {
public:
    virtual ~IGameRepository() = default;

    [[nodiscard]] virtual std::optional<int> create_game(int white_player_id,
                                                         int black_player_id) = 0;
    [[nodiscard]] virtual bool finish_game(int game_id, int winner_player_id) = 0;
    [[nodiscard]] virtual bool finish_game_without_winner(int game_id) = 0;
};

}  // namespace kfc
