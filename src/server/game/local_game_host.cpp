#include "server/game/local_game_host.h"

#include "model/piece.h"
#include "server/network/player_id.h"
#include "server/player_session.h"
#include "server/room/game_player.h"
#include "server/room/room.h"
#include "server/session/client_session_manager.h"

namespace kfc {

namespace {

const PlayerSession* find_session_by_user_id(const ClientSessionManager& session_manager,
                                               const UserId user_id) {
    for (const PlayerSession& session : session_manager.sessions()) {
        if (session.has_user() && session.user_id() == user_id) {
            return &session;
        }
    }
    return nullptr;
}

GamePlayer to_game_player(const UserId user_id, const PieceColor side, const PlayerId player_id) {
    return GamePlayer{user_id, side, player_id};
}

}  // namespace

LocalGameHost::LocalGameHost(RoomManager& room_manager, std::string game_server_id,
                             std::string endpoint)
    : room_manager_(room_manager),
      game_server_id_(std::move(game_server_id)),
      endpoint_(std::move(endpoint)) {}

void LocalGameHost::bind_session_manager(ClientSessionManager& session_manager) {
    session_manager_ = &session_manager;
}

GameCreationResponse LocalGameHost::create_room(const GameCreationRequest& request) {
    const PlayerSession* white_session =
        session_manager_ != nullptr
            ? find_session_by_user_id(*session_manager_, request.white_user_id)
            : nullptr;
    const PlayerSession* black_session =
        session_manager_ != nullptr
            ? find_session_by_user_id(*session_manager_, request.black_user_id)
            : nullptr;

    const GamePlayer white = to_game_player(
        request.white_user_id, PieceColor::White,
        white_session != nullptr ? static_cast<PlayerId>(white_session->id())
                                 : static_cast<PlayerId>(0));
    const GamePlayer black = to_game_player(
        request.black_user_id, PieceColor::Black,
        black_session != nullptr ? static_cast<PlayerId>(black_session->id())
                                 : static_cast<PlayerId>(0));

    const RoomId room_id = room_manager_.create_room();
    Room* room = room_manager_.find_room(room_id);
    room->activate(white, black);
    if (request.db_game_id.has_value()) {
        room->set_db_game_id(*request.db_game_id);
    }

    return GameCreationResponse{room_id, game_server_id_, endpoint_};
}

}  // namespace kfc
