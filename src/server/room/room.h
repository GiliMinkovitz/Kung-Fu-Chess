#pragma once

#include "logic/game_action.h"
#include "model/board_model.h"
#include "model/piece.h"
#include "server/match.h"
#include "server/room/game_player.h"
#include "server/user/user_id.h"
#include "server/room/room.h"

#include <cstdint>
#include <optional>
#include <string>

namespace kfc {

class Room {
public:
    Room(RoomId id, BoardModel board);

    [[nodiscard]] RoomId id() const noexcept;
    [[nodiscard]] bool active() const noexcept;

    void activate(const GamePlayer& white, const GamePlayer& black);
    void set_db_game_id(int id);
    void reset();

    void tick(std::int64_t delta_ms);
    void submit_action(const GameAction& action);

    [[nodiscard]] std::string generate_snapshot() const;
    [[nodiscard]] bool is_game_over() const noexcept;

    [[nodiscard]] bool contains_player(UserId user_id) const noexcept;
    [[nodiscard]] const GamePlayer* white_player() const noexcept;
    [[nodiscard]] const GamePlayer* black_player() const noexcept;
    [[nodiscard]] std::optional<int> db_game_id() const noexcept;

    [[nodiscard]] Match& match() noexcept;
    [[nodiscard]] const Match& match() const noexcept;

private:
    RoomId id_;
    BoardModel default_board_;
    Match match_;
    bool active_ = false;
    std::optional<GamePlayer> white_player_;
    std::optional<GamePlayer> black_player_;
    std::optional<int> db_game_id_;
};

}  // namespace kfc
