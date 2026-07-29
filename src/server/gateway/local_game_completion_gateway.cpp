#include "server/gateway/local_game_completion_gateway.h"

#include "server/client_connection.h"
#include "server/game_result_message_writer.h"
#include "server/network/i_message_sink.h"
#include "server/player_session.h"
#include "server/session/client_session_manager.h"
#include "server/session_registry.h"

#include <string>

namespace kfc {

LocalGameCompletionGateway::LocalGameCompletionGateway(SessionRegistry& session_registry,
                                                         ClientSessionManager& session_manager,
                                                         IMessageSink& message_sink)
    : session_registry_(session_registry),
      session_manager_(session_manager),
      message_sink_(message_sink) {}

bool LocalGameCompletionGateway::is_player_connected(const PlayerId player_id) const {
    const PlayerSession* session = session_manager_.find_session(player_id);
    return session != nullptr && session->connection() != nullptr &&
           session->connection()->is_open();
}

void LocalGameCompletionGateway::notify_game_finished(PlayerId white, PlayerId black,
                                                      PieceColor winner_color, FinishReason reason,
                                                      const RatingChange& rating_change) {
    const bool white_won = winner_color == PieceColor::White;
    const std::string white_message = create_game_result_message(
        white_won, reason,
        white_won ? rating_change.winner_new_rating : rating_change.loser_new_rating);
    const std::string black_message = create_game_result_message(
        !white_won, reason,
        white_won ? rating_change.loser_new_rating : rating_change.winner_new_rating);

    if (is_player_connected(white)) {
        (void)message_sink_.send_message(white, white_message);
    }
    if (is_player_connected(black)) {
        (void)message_sink_.send_message(black, black_message);
    }
}

void LocalGameCompletionGateway::cleanup_finished_player(const FinishedPlayerState& player) {
    PlayerSession* session = session_manager_.find_session(player.player_id);
    if (session == nullptr) {
        return;
    }

    session->clear_side();
    session->clear_room();
    if (player.has_user) {
        session->assign_user(player.user_id, player.username, player.rating);
    }
    session_registry_.unregister_session(session->player().username());
}

void LocalGameCompletionGateway::cleanup_finished_players(const FinishedPlayerState& white,
                                                            const FinishedPlayerState& black) {
    cleanup_finished_player(white);
    cleanup_finished_player(black);
}

}  // namespace kfc
