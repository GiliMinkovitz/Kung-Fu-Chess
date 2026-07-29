#pragma once

#include "server/game_result_message_writer.h"
#include "server/rating_service.h"
#include "server/room/room_manager.h"

#include "database/i_game_repository.h"
#include "model/piece.h"
#include "server/database/i_user_repository.h"

#include <optional>

namespace kfc {

class ClientSessionManager;
class GamePlayer;
class IMessageSink;
class Room;
class SessionRegistry;
class IRuntimeStore;

class GameResultHandler {
public:
    GameResultHandler(RoomManager& room_manager, IUserRepository& user_repository,
                      IGameRepository& game_repository, SessionRegistry& session_registry,
                      IRuntimeStore& runtime_store, ClientSessionManager& session_manager,
                      IMessageSink& message_sink);

    void finish(RoomId room_id, std::optional<PieceColor> winner_color, FinishReason reason);

private:
    [[nodiscard]] bool is_player_connected(PlayerId player_id) const;
    [[nodiscard]] const GamePlayer* find_player_by_color(const Room& room, PieceColor color) const;
    [[nodiscard]] std::optional<RatingChange> update_ratings_for_result(const Room& room,
                                                                        PieceColor winner_color,
                                                                        int game_id);
    void cleanup_finished_room(RoomId room_id);
    void refresh_session_player(class PlayerSession& session);

    RoomManager& room_manager_;
    IUserRepository& user_repository_;
    IGameRepository& game_repository_;
    SessionRegistry& session_registry_;
    IRuntimeStore& runtime_store_;
    ClientSessionManager& session_manager_;
    IMessageSink& message_sink_;
    RatingService rating_service_;
};

}  // namespace kfc
