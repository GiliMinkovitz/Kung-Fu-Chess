#pragma once

#include "logic/game_action.h"
#include "model/piece.h"
#include "server/network/player_id.h"
#include "server/room/room.h"

#include <optional>
#include <vector>

namespace kfc {

class ClientSessionManager;
class GamePlayer;
class Room;

struct RoomPlayerInput {
    enum class Kind { Action, Resign };

    PieceColor player_side;
    Kind kind;
    GameAction action;
};

class GameInputDispatcher {
public:
    explicit GameInputDispatcher(ClientSessionManager& session_manager);

    void probe_room(const Room& room);
    [[nodiscard]] bool is_player_connected(PlayerId player_id) const;
    [[nodiscard]] std::optional<PieceColor> disconnected_player_color(const Room& room) const;
    [[nodiscard]] bool both_room_players_disconnected(const Room& room) const;
    [[nodiscard]] std::vector<RoomPlayerInput> poll_room_inputs(const Room& room);

private:
    void probe_player(PlayerId player_id);
    void poll_player_input(const Room& room, const GamePlayer& player,
                           std::vector<RoomPlayerInput>& inputs);

    ClientSessionManager& session_manager_;
};

}  // namespace kfc
