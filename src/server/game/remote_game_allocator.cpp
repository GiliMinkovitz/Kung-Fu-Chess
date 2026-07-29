#include "server/game/remote_game_allocator.h"

#include "app/observability/metric_counters.h"
#include "server/game/game_allocation_http_client.h"
#include "server/game/i_game_server_registry.h"

#include <stdexcept>

namespace kfc {

RemoteGameAllocator::RemoteGameAllocator(IGameServerRegistry& game_server_registry,
                                         std::string service_token,
                                         const std::chrono::milliseconds timeout,
                                         const std::size_t max_retry_count)
    : game_server_registry_(game_server_registry),
      service_token_(std::move(service_token)),
      timeout_(timeout),
      max_retry_count_(max_retry_count) {}

GameCreationResponse RemoteGameAllocator::allocate_game(const GameCreationRequest& request) {
    const std::vector<GameServerRecord> candidates =
        game_server_registry_.list_available_servers();
    if (candidates.empty()) {
        throw std::runtime_error("No available game servers in registry");
    }

    GameAllocationHttpClient http_client(service_token_, timeout_);
    std::size_t attempts = 0;
    for (const GameServerRecord& server : candidates) {
        if (attempts >= max_retry_count_) {
            break;
        }

        const std::optional<GameCreationResponse> response =
            http_client.allocate(server.allocation_endpoint, request);
        ++attempts;
        if (!response.has_value()) {
            kfc::app::observability::metrics().allocation_failures_total.fetch_add(
                1, std::memory_order_relaxed);
            continue;
        }

        GameCreationResponse result = *response;
        if (result.game_server_id.empty()) {
            result.game_server_id = server.server_id;
        }
        if (!result.endpoint.has_value() || result.endpoint->empty()) {
            result.endpoint = server.endpoint;
        }
        return result;
    }

    kfc::app::observability::metrics().allocation_failures_total.fetch_add(1, std::memory_order_relaxed);
    throw std::runtime_error("Game allocation failed for all candidate servers");
}

}  // namespace kfc
