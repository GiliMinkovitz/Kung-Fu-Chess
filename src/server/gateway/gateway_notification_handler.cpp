#include "server/gateway/gateway_notification_handler.h"

#include "model/piece.h"
#include "server/gateway/game_redirect_info.h"
#include "server/gateway/local_game_gateway.h"
#include "server/network/player_id.h"
#include "server/player_session.h"
#include "server/session/client_session_manager.h"

namespace kfc {

GatewayNotificationHandler::GatewayNotificationHandler(ClientSessionManager& session_manager,
                                                       LocalGameGateway& game_gateway)
    : session_manager_(session_manager), game_gateway_(game_gateway) {}

void GatewayNotificationHandler::notify_match_found(
    const matchmaking::MatchNotification& notification) {
    PlayerSession* white = session_manager_.find_session(notification.white_player_id);
    PlayerSession* black = session_manager_.find_session(notification.black_player_id);
    if (white == nullptr || black == nullptr) {
        return;
    }

    white->set_playing();
    black->set_playing();
    white->set_side(PieceColor::White);
    black->set_side(PieceColor::Black);
    white->assign_room(notification.room_id);
    black->assign_room(notification.room_id);

    game_gateway_.notify_match_found(notification.white_player_id, notification.black_player_id);
    game_gateway_.notify_game_start(notification.white_player_id, notification.black_player_id);

    game_gateway_.send_game_redirect(
        notification.white_player_id,
        GatewayGameRedirectInfo{notification.room_id, notification.server_id, notification.endpoint,
                                PieceColor::White});
    game_gateway_.send_game_redirect(
        notification.black_player_id,
        GatewayGameRedirectInfo{notification.room_id, notification.server_id, notification.endpoint,
                                PieceColor::Black});
}

void GatewayNotificationHandler::notify_search_timeout(const PlayerId player_id) {
    if (PlayerSession* session = session_manager_.find_session(player_id)) {
        session->cancel_search();
    }
    game_gateway_.notify_search_timeout(player_id);
}

}  // namespace kfc
