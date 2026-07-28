#pragma once

#include "logic/game_action.h"
#include "model/board_model.h"
#include "server/match.h"
#include "server/player.h"

#include <cstdint>
#include <optional>
#include <string>

namespace kfc {

using RoomId = std::uint64_t;

class PlayerSession;

class Room {
public:
    Room(RoomId id, BoardModel board);

    [[nodiscard]] RoomId id() const noexcept;
    [[nodiscard]] bool active() const noexcept;

    void activate(Player* white, Player* black);
    void bind_sessions(PlayerSession* white, PlayerSession* black);
    void set_db_game_id(int id);
    void reset();

    void tick(std::int64_t delta_ms);
    void submit_action(const GameAction& action);

    [[nodiscard]] std::string generate_snapshot() const;
    [[nodiscard]] bool is_game_over() const noexcept;

    [[nodiscard]] bool contains_player(const Player* player) const noexcept;
    [[nodiscard]] Player* white_player() noexcept;
    [[nodiscard]] Player* black_player() noexcept;
    [[nodiscard]] const Player* white_player() const noexcept;
    [[nodiscard]] const Player* black_player() const noexcept;

    [[nodiscard]] PlayerSession* white_session() noexcept;
    [[nodiscard]] PlayerSession* black_session() noexcept;
    [[nodiscard]] const PlayerSession* white_session() const noexcept;
    [[nodiscard]] const PlayerSession* black_session() const noexcept;
    [[nodiscard]] std::optional<int> db_game_id() const noexcept;

    [[nodiscard]] Match& match() noexcept;
    [[nodiscard]] const Match& match() const noexcept;

private:
    RoomId id_;
    BoardModel default_board_;
    Match match_;
    bool active_ = false;
    Player* white_player_ = nullptr;
    Player* black_player_ = nullptr;
    PlayerSession* white_session_ = nullptr;
    PlayerSession* black_session_ = nullptr;
    std::optional<int> db_game_id_;
};

}  // namespace kfc
