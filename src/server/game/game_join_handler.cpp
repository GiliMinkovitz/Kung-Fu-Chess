#include "server/game/game_join_handler.h"

#include "model/piece.h"
#include "server/database/i_user_repository.h"
#include "server/game_message_parser.h"
#include "server/network/player_id.h"
#include "server/player_session.h"
#include "server/room/game_player.h"
#include "server/room/room.h"
#include "server/room/room_manager.h"
#include "server/session/client_session_manager.h"

namespace kfc {

namespace {

bool is_player_connected(const ClientSessionManager& session_manager, const PlayerId player_id) {
    const PlayerSession* session = session_manager.find_session(player_id);
    return session != nullptr && session->connection() != nullptr &&
           session->connection()->is_open();
}

std::optional<const GamePlayer*> find_reconnect_slot(const Room& room,
                                                    const ClientSessionManager& session_manager) {
    const GamePlayer* white = room.white_player();
    const GamePlayer* black = room.black_player();
    if (white == nullptr || black == nullptr) {
        return std::nullopt;
    }

    const bool white_open = !is_player_connected(session_manager, white->player_id);
    const bool black_open = !is_player_connected(session_manager, black->player_id);
    if (white_open && !black_open) {
        return white;
    }
    if (black_open && !white_open) {
        return black;
    }
    if (white_open && black_open) {
        return white;
    }
    return std::nullopt;
}

}  // namespace

GameJoinHandler::GameJoinHandler(RoomManager& room_manager,
                                 ClientSessionManager& session_manager,
                                 IUserRepository& user_repository)
    : room_manager_{room_manager},
      session_manager_{session_manager},
      user_repository_{user_repository} {}

void GameJoinHandler::process() {
    for (PlayerSession& session : session_manager_.sessions()) {
        if (session.has_room() || !session.connection()->is_open()) {
            continue;
        }

        if (const auto raw_message = session.connection()->try_read()) {
            const std::optional<RoomId> room_id = parse_join_game_message(*raw_message);
            if (!room_id.has_value()) {
                continue;
            }

            Room* room = room_manager_.find_room(*room_id);
            if (room == nullptr || !room->active()) {
                continue;
            }

            const std::optional<const GamePlayer*> slot =
                find_reconnect_slot(*room, session_manager_);
            if (!slot.has_value()) {
                continue;
            }

            const std::optional<Player> profile =
                user_repository_.find_profile_by_id((*slot)->user_id);
            if (!profile.has_value()) {
                continue;
            }

            session.assign_user((*slot)->user_id, profile->username(), profile->rating());
            session.bind_player(*profile);
            session.set_playing();
            session.set_side((*slot)->side);
            session.assign_room(*room_id);
            room->rebind_player((*slot)->user_id, static_cast<PlayerId>(session.id()));
        }
    }
}

}  // namespace kfc
