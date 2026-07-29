#pragma once

#include "server/gateway/game_redirect_info.h"
#include "server/game/protocol/game_creation_response.h"
#include "server/matchmaking/match_created_handler.h"
#include "server/user/user_id.h"

#include <string>

namespace kfc {

class IGameAllocator;
class IGameGateway;
class IGameRepository;
class IRuntimeStore;
class MatchmakingService;
class PlayerSession;

class MatchLifecycleHandler : public IMatchCreatedHandler {
public:
    MatchLifecycleHandler(IGameAllocator& game_allocator, IGameRepository& game_repository,
                          IRuntimeStore& runtime_store, std::string server_id);

    void bind_matchmaking_service(MatchmakingService& matchmaking_service);
    void bind_game_gateway(IGameGateway& game_gateway);

    RoomId create_match(PlayerSession* white, PlayerSession* black) override;
    void notify_match_created(const MatchCreated& match);
    void process_timeouts();

private:
    struct ResolvedRouting {
        RoomId room_id;
        std::string server_id;
        std::string endpoint;
    };

    [[nodiscard]] ResolvedRouting resolve_routing(const GameCreationResponse& response,
                                                  UserId user_id) const;
    void send_game_redirects(PlayerSession* white, PlayerSession* black,
                             const GameCreationResponse& response);

    IGameAllocator& game_allocator_;
    IGameRepository& game_repository_;
    IRuntimeStore& runtime_store_;
    std::string server_id_;
    MatchmakingService* matchmaking_service_ = nullptr;
    IGameGateway* game_gateway_ = nullptr;
};

}  // namespace kfc
