#include "server/game_server.h"

#include "app/runtime_diagnostics.h"

#include "model/game_config.h"

#include <chrono>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>

namespace kfc {

GameServer::GameServer(const app::AppConfig& config, BoardModel default_board,
                       app::GameServerDependencies dependencies)
    : websocket_server_(config.server),
      room_manager_(std::move(default_board)),
      match_lifecycle_handler_(room_manager_, dependencies.game_repository),
      matchmaking_service_(match_lifecycle_handler_, config.matchmaking),
      active_room_processor_(room_manager_),
      user_repository_(dependencies.user_repository),
      session_manager_(websocket_server_, session_registry_, matchmaking_service_),
      lobby_handler_(dependencies.authentication_service, matchmaking_service_, session_registry_,
                     session_manager_, match_lifecycle_handler_),
      game_result_handler_(room_manager_, user_repository_, dependencies.game_repository,
                           session_registry_) {
    app::configure_logging(config.logging);
    match_lifecycle_handler_.bind_matchmaking_service(matchmaking_service_);
    last_tick_ = std::chrono::steady_clock::now();
}

GameServer::~GameServer() = default;

WebSocketServer& GameServer::websocket_server() noexcept {
    return websocket_server_;
}

MatchmakingService& GameServer::matchmaking_service() noexcept {
    return matchmaking_service_;
}

RoomManager& GameServer::room_manager() noexcept {
    return room_manager_;
}

IUserRepository& GameServer::user_repository() noexcept {
    return user_repository_;
}

app::ServerMetrics GameServer::metrics() const {
    app::ServerMetrics result;
    result.active_rooms = room_manager_.active_room_count();
    result.connected_sessions = session_manager_.sessions().size();
    result.matchmaking_queue = matchmaking_service_.waiting_count();

    const auto now = std::chrono::steady_clock::now();
    if (started_at_.time_since_epoch().count() != 0) {
        result.server_uptime_seconds =
            std::chrono::duration_cast<std::chrono::seconds>(now - started_at_).count();
    }
    result.last_tick_duration_ms = last_tick_duration_ms_;
    return result;
}

void GameServer::tick_once() {
    session_manager_.accept_new_clients();
    lobby_handler_.process();
    match_lifecycle_handler_.process_timeouts();

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick_).count();
    last_tick_duration_ms_ = elapsed;

    active_room_processor_.process(
        elapsed, last_tick_,
        [this](RoomId room_id, std::optional<PieceColor> winner_color, FinishReason reason) {
            game_result_handler_.finish(room_id, winner_color, reason);
        });

    session_manager_.prune_sessions();
    room_manager_.remove_inactive_rooms();

    if (elapsed < kTargetFrameMs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void GameServer::run() {
#ifndef KFC_TEST_BUILD
    std::cout << "Server started\n";
#endif
    last_tick_ = std::chrono::steady_clock::now();
    started_at_ = last_tick_;

    while (!stop_requested_) {
        tick_once();
    }
}

}  // namespace kfc
