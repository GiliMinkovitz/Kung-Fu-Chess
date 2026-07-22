#pragma once

#include "model/board_model.h"
#include "server/match.h"
#include "server/player_session.h"

namespace kfc {

class GameRoom {
public:
    explicit GameRoom(BoardModel board);

    [[nodiscard]] bool active() const noexcept;
    void activate(PlayerSession* white, PlayerSession* black);
    void reset();

    [[nodiscard]] bool contains(const PlayerSession* session) const noexcept;
    [[nodiscard]] PlayerSession* white_player() noexcept;
    [[nodiscard]] PlayerSession* black_player() noexcept;
    [[nodiscard]] const PlayerSession* white_player() const noexcept;
    [[nodiscard]] const PlayerSession* black_player() const noexcept;

    [[nodiscard]] Match& match() noexcept;
    [[nodiscard]] const Match& match() const noexcept;

private:
    BoardModel default_board_;
    Match match_;
    bool active_ = false;
    PlayerSession* white_player_ = nullptr;
    PlayerSession* black_player_ = nullptr;
};

}  // namespace kfc
