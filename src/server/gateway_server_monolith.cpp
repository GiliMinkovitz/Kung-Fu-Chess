#include "server/gateway_server.h"

#include "app/runtime_endpoint.h"
#include "server/authentication_service.h"
#include "server/gateway/local_game_gateway.h"
#include "server/match/match_lifecycle_handler.h"

#include <chrono>

namespace kfc {

GatewayServer::GatewayServer(const app::AppConfig& config, ClientConnectionPlane& client_plane,
                             LocalGameGateway& game_gateway,
                             matchmaking::IMatchmakingJoinClient& matchmaking_client,
                             MatchLifecycleHandler& match_lifecycle_handler,
                             AuthenticationService& authentication_service,
                             IRuntimeStore& runtime_store)
    : client_plane_(client_plane),
      runtime_store_(runtime_store),
      local_game_gateway_(game_gateway),
      lobby_handler_(authentication_service, matchmaking_client, client_plane_.session_registry,
                     client_plane_.session_manager, config.server.region),
      gateway_server_id_(config.server.gateway_server_id),
      region_(config.server.region),
      game_endpoint_(app::resolve_game_endpoint(config.server)),
      redis_enabled_(config.redis.enabled),
      heartbeat_interval_(config.redis.heartbeat_interval) {
    match_lifecycle_handler.bind_game_gateway(local_game_gateway_);
    process_match_timeouts_ = [&match_lifecycle_handler]() {
        match_lifecycle_handler.process_timeouts();
    };
    last_tick_ = std::chrono::steady_clock::now();
}

}  // namespace kfc
