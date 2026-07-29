#include "server/game_result/game_result_handler.h"

#include "app/i_runtime_store.h"
#include "server/gateway/i_game_completion_gateway.h"
#include "server/player.h"
#include "server/room/game_player.h"
#include "server/room/room.h"
#include "server/user/user_id.h"

namespace kfc {

GameResultHandler::GameResultHandler(RoomManager& room_manager, IUserRepository& user_repository,
                                     IGameRepository& game_repository,
                                     IRuntimeStore& runtime_store,
                                     IGameCompletionGateway& completion_gateway)
    : room_manager_(room_manager),
      user_repository_(user_repository),
      game_repository_(game_repository),
      runtime_store_(runtime_store),
      completion_gateway_(completion_gateway) {}

void GameResultHandler::finish(RoomId room_id, std::optional<PieceColor> winner_color,
                               FinishReason reason) {
    Room* room = room_manager_.find_room(room_id);
    const GamePlayer* white = room != nullptr ? room->white_player() : nullptr;
    const GamePlayer* black = room != nullptr ? room->black_player() : nullptr;
    if (room == nullptr || white == nullptr || black == nullptr) {
        return;
    }

    std::optional<RatingChange> rating_change;
    if (room->db_game_id().has_value()) {
        if (winner_color.has_value()) {
            rating_change = update_ratings_for_result(*room, *winner_color, *room->db_game_id());
        } else if (reason == FinishReason::Disconnect) {
            game_repository_.finish_game_without_winner(*room->db_game_id());
        }
    }

    if (winner_color.has_value() && rating_change.has_value()) {
        completion_gateway_.notify_game_finished(white->player_id, black->player_id, *winner_color,
                                                 reason, *rating_change);
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

FinishedPlayerState GameResultHandler::build_finished_player_state(
    const GamePlayer& player) const {
    FinishedPlayerState state;
    state.player_id = player.player_id;
    state.user_id = player.user_id;
    if (const auto profile = user_repository_.find_profile_by_id(player.user_id)) {
        state.has_user = true;
        state.username = profile->username();
        state.rating = profile->rating();
    }
    return state;
}

void GameResultHandler::cleanup_finished_room(RoomId room_id) {
    Room* room = room_manager_.find_room(room_id);
    if (room == nullptr) {
        return;
    }

    const GamePlayer* white = room->white_player();
    const GamePlayer* black = room->black_player();
    FinishedPlayerState white_state;
    FinishedPlayerState black_state;
    if (white != nullptr) {
        white_state = build_finished_player_state(*white);
    }
    if (black != nullptr) {
        black_state = build_finished_player_state(*black);
    }

    if (white != nullptr && black != nullptr) {
        runtime_store_.unregister_room(room_id, white->user_id, black->user_id);
    }

    room->reset();

    completion_gateway_.cleanup_finished_players(white_state, black_state);
}

}  // namespace kfc
