#include "server/game_server_runtime.h"

#include "app/i_runtime_store.h"
#include "app/observability/metric_counters.h"
#include "app/runtime_endpoint.h"
#include "database/i_game_repository.h"
#include "model/game_config.h"
#include "server/database/i_user_repository.h"
#include "server/room/game_player.h"
#include "server/room/room.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace kfc {

GameServerRuntime::GameServerRuntime(const app::AppConfig& config,
                                     ClientConnectionPlane& client_plane,
                                     RoomManager& room_manager, LocalGameHost& local_game_host,
                                     GameAllocationHandler& allocation_handler,
                                     IUserRepository& user_repository,
                                     IGameRepository& game_repository,
                                     IRuntimeStore& runtime_store,
                                     const bool enable_creation_listener,
                                     const bool manage_connections)
    : client_plane_(client_plane),
      room_manager_(room_manager),
      local_game_host_(local_game_host),
      allocation_handler_(allocation_handler),
      runtime_store_(runtime_store),
      input_dispatcher_(client_plane_.session_manager),
      completion_gateway_(client_plane_.session_registry, client_plane_.session_manager,
                          client_plane_.session_message_sink),
      game_result_handler_(room_manager_, user_repository, game_repository, runtime_store_,
                           completion_gateway_),
      active_room_processor_(room_manager_, input_dispatcher_,
                             client_plane_.session_message_sink),
      game_join_handler_(room_manager_, client_plane_.session_manager),
      server_id_(config.server.server_id),
      region_(config.server.region),
      endpoint_(app::resolve_game_endpoint(config.server)),
      allocation_endpoint_(app::resolve_allocation_endpoint(config.server)),
      redis_enabled_(config.redis.enabled),
      manage_connections_(manage_connections),
      heartbeat_interval_(config.redis.heartbeat_interval) {
    local_game_host_.bind_session_manager(client_plane_.session_manager);

    if (enable_creation_listener) {
        creation_listener_ = std::make_unique<GameCreationListener>(
            config.server.bind_address, config.server.game_internal_port, allocation_handler_,
            config.allocation.internal_service_token);
        if (config.server.allocation_endpoint.empty()) {
            allocation_endpoint_ = "http://" + config.server.bind_address + ":" +
                                   std::to_string(creation_listener_->port()) + "/allocate";
        }
    }

    last_tick_ = std::chrono::steady_clock::now();
}

GameServerRuntime::~GameServerRuntime() {
    if (creation_listener_ != nullptr) {
        creation_listener_->stop();
    }
}

RoomManager& GameServerRuntime::room_manager() noexcept {
    return room_manager_;
}

app::ServerMetrics GameServerRuntime::metrics() const {
    app::ServerMetrics result;
    result.active_rooms = room_manager_.active_room_count();
    result.connected_sessions = client_plane_.session_manager.sessions().size();
    result.matchmaking_queue = 0;
    result.server_id = server_id_;
    result.region = region_;
    result.endpoint = endpoint_;
    result.allocation_endpoint = allocation_endpoint_;
    result.redis_enabled = redis_enabled_;
    result.redis_connected = redis_enabled_ && runtime_store_.is_available();

    const auto now = std::chrono::steady_clock::now();
    if (started_at_.time_since_epoch().count() != 0) {
        result.server_uptime_seconds =
            std::chrono::duration_cast<std::chrono::seconds>(now - started_at_).count();
    }
    result.last_tick_duration_ms = last_tick_duration_ms_;
    return result;
}

std::size_t GameServerRuntime::active_player_count() const {
    std::size_t count = 0;
    for (Room* room : room_manager_.active_rooms()) {
        if (room->white_player() != nullptr) {
            ++count;
        }
        if (room->black_player() != nullptr) {
            ++count;
        }
    }
    return count;
}

bool GameServerRuntime::is_allocation_api_active() const {
    return creation_listener_ != nullptr && creation_listener_->is_active();
}

void GameServerRuntime::maybe_publish_heartbeat() {
    const auto now = std::chrono::steady_clock::now();
    if (last_heartbeat_at_.time_since_epoch().count() != 0 &&
        now - last_heartbeat_at_ < heartbeat_interval_) {
        return;
    }

    runtime_store_.publish_server_heartbeat(server_id_, region_, metrics());
    last_heartbeat_at_ = now;
}

void GameServerRuntime::finish_room_internal(const RoomId room_id,
                                             const std::optional<PieceColor> winner_color,
                                             const FinishReason reason) {
    game_result_handler_.finish(room_id, winner_color, reason);
}

#ifdef KFC_TEST_BUILD
void GameServerRuntime::finish_room(const RoomId room_id,
                                    const std::optional<PieceColor> winner_color,
                                    const FinishReason reason) {
    finish_room_internal(room_id, winner_color, reason);
}
#endif

void GameServerRuntime::tick_once() {
    if (manage_connections_) {
        client_plane_.session_manager.accept_new_clients();
    }

    game_join_handler_.process();

    if (creation_listener_ != nullptr) {
        creation_listener_->poll();
    }

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick_).count();
    last_tick_duration_ms_ = elapsed;

    kfc::app::observability::record_tick_duration_ms(static_cast<std::uint64_t>(elapsed));

    active_room_processor_.process(
        elapsed, last_tick_,
        [this](const RoomId room_id, const std::optional<PieceColor> winner_color,
               const FinishReason reason) { finish_room_internal(room_id, winner_color, reason); });

    room_manager_.remove_inactive_rooms();

    if (manage_connections_) {
        client_plane_.session_manager.prune_sessions();
    }

    maybe_publish_heartbeat();

    if (elapsed < kTargetFrameMs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void GameServerRuntime::run() {
#ifndef KFC_TEST_BUILD
    std::cout << "Game server runtime started\n";
#endif
    last_tick_ = std::chrono::steady_clock::now();
    started_at_ = last_tick_;
    maybe_publish_heartbeat();

    while (!stop_requested_.load(std::memory_order_relaxed)) {
        tick_once();
    }

    if (creation_listener_ != nullptr) {
        creation_listener_->stop();
    }

    runtime_store_.deregister_server(server_id_);
}

}  // namespace kfc
