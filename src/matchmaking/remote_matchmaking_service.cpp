#include "matchmaking/remote_matchmaking_service.h"

#include "app/i_runtime_store.h"
#include "server/game/protocol/game_creation_request.h"
#include "server/game/protocol/game_creation_response.h"

#include <memory>
#include <unordered_map>

namespace kfc::matchmaking {

RemoteMatchmakingService::RemoteMatchmakingService(
    IRuntimeStore& runtime_store, IGameRepository& game_repository,
    IGameAllocator& game_allocator, IGatewayNotifier& gateway_notifier,
    const app::MatchmakingConfig config, std::string gateway_server_id)
    : runtime_store_(runtime_store),
      game_repository_(game_repository),
      game_allocator_(game_allocator),
      gateway_notifier_(gateway_notifier),
      config_(config),
      gateway_server_id_(std::move(gateway_server_id)),
      default_region_("local"),
      default_queue_(runtime_store_, default_region_, config_) {}

MatchResponse RemoteMatchmakingService::enter_queue(const MatchRequest& request) {
    const std::string region = request.region.empty() ? default_region_ : request.region;
    PlayerMatchmakingQueue& queue = queue_for(region);

    QueuedPlayer incoming{request.player_id, request.user_id, request.elo,
                          PlayerMatchmakingQueue::current_epoch_seconds()};

    queue.remove(request.player_id);

    if (const std::optional<std::pair<QueuedPlayer, QueuedPlayer>> pair = queue.try_pair(incoming)) {
        finalize_match(pair->first, pair->second);
        return MatchResponse{MatchJoinStatus::Queued, "matched"};
    }

    queue.enqueue(incoming);
    return MatchResponse{MatchJoinStatus::Queued, "queued"};
}

void RemoteMatchmakingService::leave_queue(const PlayerId player_id, const std::string_view region) {
    queue_for(region).remove(player_id);
}

void RemoteMatchmakingService::process_queue(const std::string_view region) {
    for (const PlayerId player_id : queue_for(region).check_timeouts()) {
        gateway_notifier_.notify_search_timeout(player_id);
    }
}

std::vector<PlayerId> RemoteMatchmakingService::drain_timeouts(const std::string_view region) {
    std::vector<PlayerId> timed_out = queue_for(region).check_timeouts();
    for (const PlayerId player_id : timed_out) {
        gateway_notifier_.notify_search_timeout(player_id);
    }
    return timed_out;
}

std::size_t RemoteMatchmakingService::waiting_count(const std::string_view region) const {
    if (region == default_region_) {
        return default_queue_.waiting_count();
    }
    return runtime_store_.list_matchmaking_queue(region).size();
}

PlayerMatchmakingQueue& RemoteMatchmakingService::queue_for(const std::string_view region) {
    if (region == default_region_) {
        return default_queue_;
    }

    const std::string key(region);
    const auto it = region_queues_.find(key);
    if (it != region_queues_.end()) {
        return *it->second;
    }

    auto queue = std::make_unique<PlayerMatchmakingQueue>(runtime_store_, key, config_);
    PlayerMatchmakingQueue& ref = *queue;
    region_queues_.emplace(key, std::move(queue));
    return ref;
}

void RemoteMatchmakingService::finalize_match(const QueuedPlayer& white,
                                              const QueuedPlayer& black) {
    const std::optional<int> db_game_id =
        game_repository_.create_game(static_cast<int>(white.user_id),
                                     static_cast<int>(black.user_id));

    const GameCreationRequest request{white.user_id, black.user_id, db_game_id};
    const GameCreationResponse response = game_allocator_.allocate_game(request);

    const std::string routing_server_id =
        response.game_server_id.empty() ? gateway_server_id_ : response.game_server_id;
    const std::string endpoint =
        response.endpoint.has_value() ? *response.endpoint : std::string{};

    runtime_store_.register_room(response.room_id, white.user_id, black.user_id, routing_server_id,
                                 endpoint);

    MatchNotification notification;
    notification.white_player_id = white.player_id;
    notification.white_user_id = white.user_id;
    notification.black_player_id = black.player_id;
    notification.black_user_id = black.user_id;
    notification.room_id = response.room_id;
    notification.server_id = routing_server_id;
    notification.endpoint = endpoint;
    gateway_notifier_.notify_match_found(notification);
}

}  // namespace kfc::matchmaking
