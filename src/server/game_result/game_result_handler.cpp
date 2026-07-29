#include "server/game_result/game_result_handler.h"

#include "app/i_runtime_store.h"
#include "server/client_connection.h"
#include "server/game_result_message_writer.h"
#include "server/player_session.h"
#include "server/room/room.h"
#include "server/session_registry.h"
#include "server/user/user_id.h"

#include <string>

namespace kfc {

GameResultHandler::GameResultHandler(RoomManager& room_manager, IUserRepository& user_repository,
                                     IGameRepository& game_repository,
                                     SessionRegistry& session_registry,
                                     IRuntimeStore& runtime_store)
    : room_manager_(room_manager),
      user_repository_(user_repository),
      game_repository_(game_repository),
      session_registry_(session_registry),
      runtime_store_(runtime_store) {}

void GameResultHandler::finish(RoomId room_id, std::optional<PieceColor> winner_color,
                               FinishReason reason) {
    Room* room = room_manager_.find_room(room_id);
    if (room == nullptr || room->white_session() == nullptr || room->black_session() == nullptr) {
        return;
    }

    PlayerSession* white_session = room->white_session();
    PlayerSession* black_session = room->black_session();
    ClientConnection* white_connection =
        white_session != nullptr ? white_session->connection() : nullptr;
    ClientConnection* black_connection =
        black_session != nullptr ? black_session->connection() : nullptr;

    std::optional<RatingChange> rating_change;
    if (room->db_game_id().has_value()) {
        if (winner_color.has_value()) {
            rating_change = update_ratings_for_result(*room, *winner_color, *room->db_game_id());
        } else if (reason == FinishReason::Disconnect) {
            game_repository_.finish_game_without_winner(*room->db_game_id());
        }
    }

    if (winner_color.has_value() && rating_change.has_value() && white_connection != nullptr &&
        black_connection != nullptr) {
        const bool white_won = *winner_color == PieceColor::White;
        const std::string white_message = create_game_result_message(
            white_won, reason,
            white_won ? rating_change->winner_new_rating : rating_change->loser_new_rating);
        const std::string black_message = create_game_result_message(
            !white_won, reason,
            white_won ? rating_change->loser_new_rating : rating_change->winner_new_rating);

        (void)white_connection->send_message(white_message);
        (void)black_connection->send_message(black_message);
    }

    cleanup_finished_room(room_id);
}

const Player* GameResultHandler::find_player_by_color(const Room& room, PieceColor color) const {
    return color == PieceColor::White ? room.white_player() : room.black_player();
}

std::optional<RatingChange> GameResultHandler::update_ratings_for_result(
    const Room& room, PieceColor winner_color, int game_id) {
    const Player* winner = find_player_by_color(room, winner_color);
    const PieceColor loser_color =
        winner_color == PieceColor::White ? PieceColor::Black : PieceColor::White;
    const Player* loser = find_player_by_color(room, loser_color);
    if (winner == nullptr || loser == nullptr) {
        return std::nullopt;
    }

    const RatingChange change = rating_service_.calculate(winner->rating(), loser->rating());
    user_repository_.update_rating(static_cast<UserId>(winner->id()), change.winner_new_rating);
    user_repository_.update_rating(static_cast<UserId>(loser->id()), change.loser_new_rating);
    game_repository_.finish_game(game_id, winner->id());
    return change;
}

void GameResultHandler::cleanup_finished_room(RoomId room_id) {
    Room* room = room_manager_.find_room(room_id);
    if (room == nullptr) {
        return;
    }

    PlayerSession* white = room->white_session();
    PlayerSession* black = room->black_session();
    const Player* white_player = room->white_player();
    const Player* black_player = room->black_player();

    if (white_player != nullptr && black_player != nullptr) {
        runtime_store_.unregister_room(room_id, static_cast<UserId>(white_player->id()),
                                       static_cast<UserId>(black_player->id()));
    }

    room->reset();

    if (white != nullptr) {
        white->clear_side();
        white->clear_room();
        refresh_session_player(*white);
        session_registry_.unregister_session(white->player().username());
    }
    if (black != nullptr) {
        black->clear_side();
        black->clear_room();
        refresh_session_player(*black);
        session_registry_.unregister_session(black->player().username());
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
