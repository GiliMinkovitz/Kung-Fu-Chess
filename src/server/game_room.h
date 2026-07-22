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
    [[nodiscard]] Player* white_player() noexcept;
    [[nodiscard]] Player* black_player() noexcept;
    [[nodiscard]] const Player* white_player() const noexcept;
    [[nodiscard]] const Player* black_player() const noexcept;
    [[nodiscard]] PlayerSession* white_session() noexcept;
    [[nodiscard]] PlayerSession* black_session() noexcept;
    [[nodiscard]] const PlayerSession* white_session() const noexcept;
    [[nodiscard]] const PlayerSession* black_session() const noexcept;

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
