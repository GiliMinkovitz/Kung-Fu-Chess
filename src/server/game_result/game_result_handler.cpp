#include "server/game_result/game_result_handler.h"

#include "app/i_runtime_store.h"
#include "server/client_connection.h"
#include "server/game_result_message_writer.h"
#include "server/network/i_message_sink.h"
#include "server/player.h"
#include "server/player_session.h"
#include "server/room/game_player.h"
#include "server/room/room.h"
#include "server/session/client_session_manager.h"
#include "server/session_registry.h"
#include "server/user/user_id.h"

#include <string>

namespace kfc {

GameResultHandler::GameResultHandler(RoomManager& room_manager, IUserRepository& user_repository,
                                     IGameRepository& game_repository,
                                     SessionRegistry& session_registry,
                                     IRuntimeStore& runtime_store,
                                     ClientSessionManager& session_manager,
                                     IMessageSink& message_sink)
    : room_manager_(room_manager),
      user_repository_(user_repository),
      game_repository_(game_repository),
      session_registry_(session_registry),
      runtime_store_(runtime_store),
      session_manager_(session_manager),
      message_sink_(message_sink) {}

bool GameResultHandler::is_player_connected(const PlayerId player_id) const {
    const PlayerSession* session = session_manager_.find_session(player_id);
    return session != nullptr && session->connection() != nullptr &&
           session->connection()->is_open();
}

void GameResultHandler::finish(RoomId room_id, std::optional<PieceColor> winner_color,
                               FinishReason reason) {
    Room* room = room_manager_.find_room(room_id);
    const GamePlayer* white = room != nullptr ? room->white_player() : nullptr;
    const GamePlayer* black = room != nullptr ? room->black_player() : nullptr;
    if (room == nullptr || white == nullptr || black == nullptr) {
        return;
    }

    const bool white_connected = is_player_connected(white->player_id);
    const bool black_connected = is_player_connected(black->player_id);

    std::optional<RatingChange> rating_change;
    if (room->db_game_id().has_value()) {
        if (winner_color.has_value()) {
            rating_change = update_ratings_for_result(*room, *winner_color, *room->db_game_id());
        } else if (reason == FinishReason::Disconnect) {
            game_repository_.finish_game_without_winner(*room->db_game_id());
        }
    }

    if (winner_color.has_value() && rating_change.has_value() && white_connected &&
        black_connected) {
        const bool white_won = *winner_color == PieceColor::White;
        const std::string white_message = create_game_result_message(
            white_won, reason,
            white_won ? rating_change->winner_new_rating : rating_change->loser_new_rating);
        const std::string black_message = create_game_result_message(
            !white_won, reason,
            white_won ? rating_change->loser_new_rating : rating_change->winner_new_rating);

        (void)message_sink_.send_message(white->player_id, white_message);
        (void)message_sink_.send_message(black->player_id, black_message);
    }

    cleanup_finished_room(room_id);
}

const GamePlayer* GameResultHandler::find_player_by_color(const Room& room,
                                                          PieceColor color) const {
    return color == PieceColor::White ? room.white_player() : room.black_player();
}

std::optional<RatingChange> GameResultHandler::update_ratings_for_result(
    const Room& room, PieceColor winner_color, int game_id) {
    const GamePlayer* winner = find_player_by_color(room, winner_color);
    const PieceColor loser_color =
        winner_color == PieceColor::White ? PieceColor::Black : PieceColor::White;
    const GamePlayer* loser = find_player_by_color(room, loser_color);
    if (winner == nullptr || loser == nullptr) {
        return std::nullopt;
    }

    const std::optional<Player> winner_profile = user_repository_.find_profile_by_id(winner->user_id);
    const std::optional<Player> loser_profile = user_repository_.find_profile_by_id(loser->user_id);
    if (!winner_profile.has_value() || !loser_profile.has_value()) {
        return std::nullopt;
    }

    const RatingChange change =
        rating_service_.calculate(winner_profile->rating(), loser_profile->rating());
    user_repository_.update_rating(winner->user_id, change.winner_new_rating);
    user_repository_.update_rating(loser->user_id, change.loser_new_rating);
    game_repository_.finish_game(game_id, static_cast<int>(winner->user_id));
    return change;
}

void GameResultHandler::cleanup_finished_room(RoomId room_id) {
    Room* room = room_manager_.find_room(room_id);
    if (room == nullptr) {
        return;
    }

    const GamePlayer* white = room->white_player();
    const GamePlayer* black = room->black_player();
    const PlayerId white_player_id = white != nullptr ? white->player_id : 0;
    const PlayerId black_player_id = black != nullptr ? black->player_id : 0;

    if (white != nullptr && black != nullptr) {
        runtime_store_.unregister_room(room_id, white->user_id, black->user_id);
    }

    room->reset();

    if (PlayerSession* white_session = session_manager_.find_session(white_player_id)) {
        white_session->clear_side();
        white_session->clear_room();
        refresh_session_player(*white_session);
        session_registry_.unregister_session(white_session->player().username());
    }
    if (PlayerSession* black_session = session_manager_.find_session(black_player_id)) {
        black_session->clear_side();
        black_session->clear_room();
        refresh_session_player(*black_session);
        session_registry_.unregister_session(black_session->player().username());
    }
}

void GameResultHandler::refresh_session_player(PlayerSession& session) {
    if (!session.has_user()) {
        return;
    }
    if (const auto updated = user_repository_.find_profile_by_id(session.user_id())) {
        session.assign_user(session.user_id(), updated->username(), updated->rating());
    }
}

}  // namespace kfc
