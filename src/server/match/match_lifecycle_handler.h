#pragma once

#include "server/matchmaking/match_created_handler.h"

namespace kfc {

class IGameRepository;
class MatchmakingService;
class PlayerSession;
class RoomManager;

class MatchLifecycleHandler : public IMatchCreatedHandler {
public:
    MatchLifecycleHandler(RoomManager& room_manager, IGameRepository& game_repository);

    void bind_matchmaking_service(MatchmakingService& matchmaking_service);

    RoomId create_match(PlayerSession* white, PlayerSession* black) override;
    void notify_match_created(const MatchCreated& match);
    void process_timeouts();

private:
    RoomManager& room_manager_;
    IGameRepository& game_repository_;
    MatchmakingService* matchmaking_service_ = nullptr;
};

}  // namespace kfc
