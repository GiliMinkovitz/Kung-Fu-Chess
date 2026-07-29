#include "server/network/game_input_dispatcher.h"

#include "server/client_connection.h"
#include "server/game_message_parser.h"
#include "server/player_session.h"
#include "server/room/game_player.h"
#include "server/room/room.h"
#include "server/session/client_session_manager.h"

namespace kfc {

GameInputDispatcher::GameInputDispatcher(ClientSessionManager& session_manager)
    : session_manager_(session_manager) {}

void GameInputDispatcher::probe_room(const Room& room) {
    if (const GamePlayer* white = room.white_player()) {
        probe_player(white->player_id);
    }
    if (const GamePlayer* black = room.black_player()) {
        probe_player(black->player_id);
    }
}

void GameInputDispatcher::probe_player(const PlayerId player_id) {
    PlayerSession* session = session_manager_.find_session(player_id);
    if (session == nullptr || session->connection() == nullptr) {
        return;
    }

    session->connection()->probe_disconnect();
}

bool GameInputDispatcher::is_player_connected(const PlayerId player_id) const {
    const PlayerSession* session = session_manager_.find_session(player_id);
    return session != nullptr && session->connection() != nullptr &&
           session->connection()->is_open();
}

std::optional<PieceColor> GameInputDispatcher::disconnected_player_color(
    const Room& room) const {
    const GamePlayer* white = room.white_player();
    const GamePlayer* black = room.black_player();
    if (white == nullptr || black == nullptr) {
        return std::nullopt;
    }

    const bool white_connected = is_player_connected(white->player_id);
    const bool black_connected = is_player_connected(black->player_id);
    if (white_connected && black_connected) {
        return std::nullopt;
    }
    if (!white_connected && black_connected) {
        return PieceColor::White;
    }
    if (white_connected && !black_connected) {
        return PieceColor::Black;
    }
    return std::nullopt;
}

bool GameInputDispatcher::both_room_players_disconnected(const Room& room) const {
    const GamePlayer* white = room.white_player();
    const GamePlayer* black = room.black_player();
    if (white == nullptr || black == nullptr) {
        return true;
    }
    return !is_player_connected(white->player_id) && !is_player_connected(black->player_id);
}

std::vector<RoomPlayerInput> GameInputDispatcher::poll_room_inputs(const Room& room) {
    std::vector<RoomPlayerInput> inputs;

    if (const GamePlayer* white = room.white_player()) {
        poll_player_input(room, *white, inputs);
    }
    if (const GamePlayer* black = room.black_player()) {
        poll_player_input(room, *black, inputs);
    }

    return inputs;
}

void GameInputDispatcher::poll_player_input(const Room& room, const GamePlayer& player,
                                            std::vector<RoomPlayerInput>& inputs) {
    PlayerSession* session = session_manager_.find_session(player.player_id);
    if (session == nullptr || session->connection() == nullptr ||
        session->state() != PlayerSessionState::Playing || !session->has_room()) {
        return;
    }

    if (const auto raw_message = session->connection()->try_read()) {
        if (parse_resign_message(*raw_message)) {
            if (room.contains_player(player.user_id)) {
                inputs.push_back(RoomPlayerInput{player.side, RoomPlayerInput::Kind::Resign, {}});
            }
            return;
        }

        if (const auto action = parse_message(*raw_message)) {
            inputs.push_back(
                RoomPlayerInput{player.side, RoomPlayerInput::Kind::Action, *action});
        }
    }
}

}  // namespace kfc
