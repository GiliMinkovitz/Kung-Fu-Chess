#pragma once

#include "app/matchmaking_config.h"
#include "matchmaking/gateway/i_gateway_notifier.h"
#include "matchmaking/i_matchmaking_service.h"
#include "matchmaking/player_matchmaking_queue.h"

#include "database/i_game_repository.h"
#include "server/game/i_game_allocator.h"

#include <memory>
#include <unordered_map>

namespace kfc {

class IRuntimeStore;

namespace matchmaking {

class RemoteMatchmakingService final : public IMatchmakingService {
public:
    RemoteMatchmakingService(IRuntimeStore& runtime_store, IGameRepository& game_repository,
                             IGameAllocator& game_allocator, IGatewayNotifier& gateway_notifier,
                             app::MatchmakingConfig config, std::string gateway_server_id);

    MatchResponse enter_queue(const MatchRequest& request) override;
    void leave_queue(PlayerId player_id, std::string_view region) override;
    void process_queue(std::string_view region) override;
    [[nodiscard]] std::vector<PlayerId> drain_timeouts(std::string_view region) override;
    [[nodiscard]] std::size_t waiting_count(std::string_view region) const override;

private:
    [[nodiscard]] PlayerMatchmakingQueue& queue_for(std::string_view region);
    void finalize_match(const QueuedPlayer& white, const QueuedPlayer& black);

    IRuntimeStore& runtime_store_;
    IGameRepository& game_repository_;
    IGameAllocator& game_allocator_;
    IGatewayNotifier& gateway_notifier_;
    app::MatchmakingConfig config_;
    std::string gateway_server_id_;
    std::string default_region_;
    PlayerMatchmakingQueue default_queue_;
    std::unordered_map<std::string, std::unique_ptr<PlayerMatchmakingQueue>> region_queues_;
};

}  // namespace matchmaking
}  // namespace kfc
