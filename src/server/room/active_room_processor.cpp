#include "server/room/active_room_processor.h"

#include "model/game_config.h"
#include "server/game_message_parser.h"
#include "server/player_session.h"
#include "server/room/room.h"
#include "server/client_connection.h"

#include <string>

namespace {

[[nodiscard]] bool is_room_player_connected(const kfc::PlayerSession* session) {
    return session != nullptr && session->connection() != nullptr &&
           session->connection()->is_open();
}

}  // namespace

namespace kfc {

ActiveRoomProcessor::ActiveRoomProcessor(RoomManager& room_manager)
    : room_manager_(room_manager) {}

void ActiveRoomProcessor::process(std::int64_t elapsed,
                                  std::chrono::steady_clock::time_point& last_tick,
                                  FinishCallback finish_callback) {
    if (room_manager_.active_rooms().empty()) {
        return;
    }

    for (Room* room : room_manager_.active_rooms()) {
        if (room->white_session() == nullptr || room->black_session() == nullptr) {
            continue;
        }

        probe_room_connections(*room);

        if (both_room_players_disconnected(*room)) {
            finish_callback(room->id(), std::nullopt, FinishReason::Disconnect);
            continue;
        }
        if (const std::optional<PieceColor> disconnected = disconnected_player_color(*room)) {
            const PieceColor winner =
                *disconnected == PieceColor::White ? PieceColor::Black : PieceColor::White;
            finish_callback(room->id(), winner, FinishReason::Disconnect);
            continue;
        }

        if (elapsed >= kTargetFrameMs && !room->is_game_over()) {
            room->tick(elapsed);

            const std::string snapshot = room->generate_snapshot();
            if (is_room_player_connected(room->white_session())) {
                room->white_session()->connection()->try_send(snapshot);
            }
            if (is_room_player_connected(room->black_session())) {
                room->black_session()->connection()->try_send(snapshot);
            }

            last_tick = std::chrono::steady_clock::now();
        }

        if (room->active() && !room->is_game_over()) {
            process_room_player_messages_if_playing(*room, room->white_session(), finish_callback);
            process_room_player_messages_if_playing(*room, room->black_session(), finish_callback);
        }

        if (room->is_game_over()) {
            finish_callback(room->id(), room->match().state().winning_color(),
                            FinishReason::KingCapture);
        }
    }
}

void ActiveRoomProcessor::probe_room_connections(Room& room) {
    if (PlayerSession* white_session = room.white_session()) {
        if (ClientConnection* connection = white_session->connection()) {
            connection->probe_disconnect();
        }
    }
    if (PlayerSession* black_session = room.black_session()) {
        if (ClientConnection* connection = black_session->connection()) {
            connection->probe_disconnect();
        }
    }
}

std::optional<PieceColor> ActiveRoomProcessor::disconnected_player_color(
    const Room& room) const {
    if (room.white_session() == nullptr || room.black_session() == nullptr) {
        return std::nullopt;
    }

    const bool white_connected = is_room_player_connected(room.white_session());
    const bool black_connected = is_room_player_connected(room.black_session());
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

bool ActiveRoomProcessor::both_room_players_disconnected(const Room& room) const {
    return !is_room_player_connected(room.white_session()) &&
           !is_room_player_connected(room.black_session());
}

void ActiveRoomProcessor::process_room_player_messages_if_playing(
    Room& room, PlayerSession* session, FinishCallback& finish_callback) {
    if (session == nullptr || session->state() != PlayerSessionState::Playing ||
        !session->has_room()) {
        return;
    }
    process_room_player_messages(room, *session, finish_callback);
}

void ActiveRoomProcessor::process_room_player_messages(Room& room, PlayerSession& session,
                                                       FinishCallback& finish_callback) {
    if (const auto raw_message = session.connection()->try_read()) {
        if (parse_resign_message(*raw_message)) {
            if (room.contains_player(&session.player()) && session.has_side()) {
                const PieceColor winner =
                    session.side() == PieceColor::White ? PieceColor::Black : PieceColor::White;
                finish_callback(room.id(), winner, FinishReason::Resign);
            }
            return;
        }

        if (const auto action = parse_message(*raw_message)) {
            if (is_action_allowed(session, room.match(), *action)) {
                room.submit_action(*action);
            }
        }
    }
}

}  // namespace kfc
