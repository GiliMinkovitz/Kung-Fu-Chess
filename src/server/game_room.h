#pragma once

#include "model/board_model.h"
#include "server/match.h"
#include "server/player_session.h"

#include <vector>

namespace kfc {

class GameRoom {
public:
    GameRoom(BoardModel board, std::vector<PlayerSession*> players);

    [[nodiscard]] Match& match() noexcept;
    [[nodiscard]] const Match& match() const noexcept;
    [[nodiscard]] const std::vector<PlayerSession*>& players() const noexcept;

private:
    Match match_;
    std::vector<PlayerSession*> players_;
};

}  // namespace kfc
