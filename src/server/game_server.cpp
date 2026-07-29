#include "server/game_server.h"

#include "app/i_runtime_store.h"
#include "app/runtime_diagnostics.h"
#include "app/runtime_endpoint.h"

#include <chrono>
#include <iostream>

namespace kfc {

namespace {

app::ServerConfig make_monolithic_websocket_config(const app::AppConfig& config) {
    app::ServerConfig websocket_config = config.server;
    websocket_config.port = config.server.port;
    websocket_config.max_clients = config.server.max_clients;
    return websocket_config;
}

}  // namespace

GameServer::GameServer(const app::AppConfig& config, BoardModel default_board,
                       app::GameServerDependencies dependencies)
    : room_manager_(std::move(default_board)),
      local_game_host_(room_manager_, config.server.server_id,
                       app::resolve_game_endpoint(config.server)),
      allocation_handler_(local_game_host_, dependencies.runtime_store, config.server.server_id,
                          app::resolve_game_endpoint(config.server)),
      local_game_allocator_(local_game_host_),
      match_lifecycle_handler_(local_game_allocator_, dependencies.game_repository,
                               dependencies.runtime_store, config.server.server_id),
      matchmaking_service_(match_lifecycle_handler_, config.matchmaking),
      disconnect_handler_(matchmaking_service_),
      client_plane_(make_monolithic_websocket_config(config), &disconnect_handler_),
      matchmaking_join_client_(matchmaking_service_, match_lifecycle_handler_,
                               client_plane_.session_manager),
      local_game_gateway_(client_plane_.session_message_sink),
      gateway_(config, client_plane_, local_game_gateway_, matchmaking_join_client_,
               match_lifecycle_handler_, dependencies.authentication_service,
               dependencies.runtime_store),
      runtime_(config, client_plane_, room_manager_, local_game_host_, allocation_handler_,
               dependencies.user_repository, dependencies.game_repository, dependencies.runtime_store,
               false, false),
      user_repository_(dependencies.user_repository),
      runtime_store_(dependencies.runtime_store) {
    app::configure_logging(config.logging);
    match_lifecycle_handler_.bind_matchmaking_service(matchmaking_service_);
    local_game_host_.bind_session_manager(client_plane_.session_manager);
}

GameServer::~GameServer() = default;

void GameServer::request_stop() noexcept {
    stop_requested_.store(true, std::memory_order_relaxed);
}

WebSocketServer& GameServer::websocket_server() noexcept {
    return gateway_.websocket_server();
}

MatchmakingService& GameServer::matchmaking_service() noexcept {
    return matchmaking_service_;
}

RoomManager& GameServer::room_manager() noexcept {
    return runtime_.room_manager();
}

IUserRepository& GameServer::user_repository() noexcept {
    return user_repository_;
}

IRuntimeStore& GameServer::runtime_store() noexcept {
    return runtime_store_;
}

app::ServerMetrics GameServer::metrics() const {
    app::ServerMetrics result = gateway_.metrics();
    const app::ServerMetrics runtime_metrics = runtime_.metrics();
    result.active_rooms = runtime_metrics.active_rooms;
    result.matchmaking_queue = matchmaking_service_.waiting_count();
    result.server_id = runtime_metrics.server_id;
    result.endpoint = runtime_metrics.endpoint;
    return result;
}

void GameServer::tick_once() {
    gateway_.tick_once();
    runtime_.tick_once();
}

void GameServer::run() {
#ifndef KFC_TEST_BUILD
    std::cout << "Server started\n";
#endif

    while (!stop_requested_.load(std::memory_order_relaxed)) {
        tick_once();
    }

    runtime_store_.deregister_server(gateway_.metrics().server_id);
}

#ifdef KFC_TEST_BUILD
void GameServer::finish_room(const RoomId room_id, const std::optional<PieceColor> winner_color,
                             const FinishReason reason) {
    runtime_.finish_room(room_id, winner_color, reason);
}
#endif

}  // namespace kfc
