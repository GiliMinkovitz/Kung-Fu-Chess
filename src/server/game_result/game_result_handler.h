#pragma once

#include "server/game_result_message_writer.h"
#include "server/gateway/i_game_completion_gateway.h"
#include "server/rating_service.h"
#include "server/room/room_manager.h"

#include "database/i_game_repository.h"
#include "model/piece.h"
#include "server/database/i_user_repository.h"

#include <optional>

namespace kfc {

class GamePlayer;
class Room;
class IRuntimeStore;

class GameResultHandler {
public:
    GameResultHandler(RoomManager& room_manager, IUserRepository& user_repository,
                      IGameRepository& game_repository, IRuntimeStore& runtime_store,
                      IGameCompletionGateway& completion_gateway);

    void finish(RoomId room_id, std::optional<PieceColor> winner_color, FinishReason reason);

private:
    [[nodiscard]] const GamePlayer* find_player_by_color(const Room& room, PieceColor color) const;
    [[nodiscard]] std::optional<RatingChange> update_ratings_for_result(const Room& room,
                                                                        PieceColor winner_color,
                                                                        int game_id);
    [[nodiscard]] FinishedPlayerState build_finished_player_state(const GamePlayer& player) const;
    void cleanup_finished_room(RoomId room_id);

    RoomManager& room_manager_;
    IUserRepository& user_repository_;
    IGameRepository& game_repository_;
    IRuntimeStore& runtime_store_;
    IGameCompletionGateway& completion_gateway_;
    RatingService rating_service_;
};

}  // namespace kfc
