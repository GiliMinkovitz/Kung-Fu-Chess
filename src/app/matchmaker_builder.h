#pragma once

#include "app/app_config.h"
#include "app/server_infrastructure.h"
#include "matchmaking/http/gateway_notifier_http_client.h"
#include "matchmaking/matchmaker_runtime.h"
#include "server/game/remote_game_allocator.h"
#include "server/game/runtime_store_game_server_registry.h"

namespace kfc::app {

struct BuiltMatchmaker {
    ServerInfrastructure infrastructure;
    RuntimeStoreGameServerRegistry game_server_registry;
    RemoteGameAllocator game_allocator;
    matchmaking::GatewayNotifierHttpClient gateway_notifier;
    matchmaking::MatchmakerRuntime runtime;

    BuiltMatchmaker(const AppConfig& config)
        : infrastructure(config),
          game_server_registry(infrastructure.runtime_store(), config.allocation.game_server_ttl),
          game_allocator(game_server_registry, config.allocation.internal_service_token,
                         config.allocation.allocation_timeout,
                         config.allocation.allocation_retry_count),
          gateway_notifier(config.server.gateway_notification_endpoint,
                           config.allocation.allocation_timeout),
          runtime(config, infrastructure.runtime_store(), game_allocator,
                  infrastructure.game_repository(), gateway_notifier) {}
};

[[nodiscard]] inline BuiltMatchmaker build_matchmaker(const AppConfig& config) {
    return BuiltMatchmaker{config};
}

}  // namespace kfc::app
