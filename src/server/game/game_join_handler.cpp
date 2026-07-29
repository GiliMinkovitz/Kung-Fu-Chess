#include "server/game/game_join_handler.h"

#include "model/piece.h"
#include "server/game_message_parser.h"
#include "server/network/player_id.h"
#include "server/player_session.h"
#include "server/room/game_player.h"
#include "server/room/room.h"

namespace kfc {

GameJoinHandler::GameJoinHandler(RoomManager& room_manager,
                                 ClientSessionManager& session_manager)
    : room_manager_(room_manager), session_manager_(session_manager) {}

void GameJoinHandler::process() {
    for (PlayerSession& session : session_manager_.sessions()) {
        if (!session.connection()->is_open()) {
            continue;
        }

        if (const auto raw_message = session.connection()->try_read()) {
            const std::optional<RoomId> room_id = parse_join_game_message(*raw_message);
            if (!room_id.has_value()) {
                continue;
            }

            Room* room = room_manager_.find_room(*room_id);
            if (room == nullptr || !room->active()) {
                session.connection()->try_send("join_failed room_not_found");
                continue;
            }

            if (!session.has_user()) {
                session.connection()->try_send("join_failed not_authenticated");
                continue;
            }

            const UserId user_id = session.user_id();
            const GamePlayer* white = room->white_player();
            const GamePlayer* black = room->black_player();
            if (white == nullptr || black == nullptr) {
                session.connection()->try_send("join_failed room_not_ready");
                continue;
            }

            if (white->user_id == user_id) {
                session.assign_room(*room_id);
                session.set_playing();
                session.set_side(PieceColor::White);
                session.connection()->try_send("join_ok white");
                continue;
            }

            if (black->user_id == user_id) {
                session.assign_room(*room_id);
                session.set_playing();
                session.set_side(PieceColor::Black);
                session.connection()->try_send("join_ok black");
                continue;
            }

            session.connection()->try_send("join_failed not_a_player");
        }
    }
}

}  // namespace kfc
