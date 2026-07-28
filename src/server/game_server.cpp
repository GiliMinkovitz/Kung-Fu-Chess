#include "server/game_server.h"

#include "model/game_config.h"

#include <chrono>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>

namespace kfc {

GameServer::GameServer(const app::ServerConfig& server_config, BoardModel default_board,
                       app::GameServerDependencies dependencies)
    : websocket_server_(server_config),
      room_manager_(std::move(default_board)),
      match_lifecycle_handler_(room_manager_, dependencies.game_repository),
      matchmaking_service_(match_lifecycle_handler_),
      active_room_processor_(room_manager_),
      user_repository_(dependencies.user_repository),
      session_manager_(websocket_server_, session_registry_, matchmaking_service_),
      lobby_handler_(dependencies.authentication_service, matchmaking_service_, session_registry_,
                     session_manager_, match_lifecycle_handler_),
      game_result_handler_(room_manager_, user_repository_, dependencies.game_repository,
                           session_registry_) {
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

void GameServer::tick_once() {
    session_manager_.accept_new_clients();
    lobby_handler_.process();
    match_lifecycle_handler_.process_timeouts();

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick_).count();

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

#ifdef KFC_TEST_BUILD
    while (!stop_requested_) {
#else
    while (true) {
#endif
        tick_once();
    }
}

}  // namespace kfc
