#include "app/in_memory_runtime_store.h"

namespace kfc {

InMemoryRuntimeStore::InMemoryRuntimeStore(std::string game_server_endpoint)
    : game_server_endpoint_(std::move(game_server_endpoint)) {}

bool InMemoryRuntimeStore::is_available() const {
    return false;
}

void InMemoryRuntimeStore::register_room(const RoomId room_id, const UserId white_player_id,
                                         const UserId black_player_id,
                                         const std::string_view server_id) {
    const PlayerLocationEntry entry{room_id, std::string(server_id), game_server_endpoint_};
    player_locations_[white_player_id] = entry;
    player_locations_[black_player_id] = entry;
}

void InMemoryRuntimeStore::unregister_room(const RoomId, const UserId white_player_id,
                                           const UserId black_player_id) {
    player_locations_.erase(white_player_id);
    player_locations_.erase(black_player_id);
}

void InMemoryRuntimeStore::publish_server_heartbeat(const std::string_view server_id,
                                                    const std::string_view,
                                                    const app::ServerMetrics& metrics) {
    if (!metrics.endpoint.empty()) {
        server_endpoints_[std::string(server_id)] = metrics.endpoint;
    }
}

void InMemoryRuntimeStore::deregister_server(const std::string_view server_id) {
    server_endpoints_.erase(std::string(server_id));
}

std::optional<GameServerLocation> InMemoryRuntimeStore::find_player_location(
    const UserId user_id) const {
    const auto it = player_locations_.find(user_id);
    if (it == player_locations_.end()) {
        return std::nullopt;
    }

    std::string endpoint = it->second.endpoint;
    if (endpoint.empty()) {
        const auto server_it = server_endpoints_.find(it->second.server_id);
        if (server_it != server_endpoints_.end()) {
            endpoint = server_it->second;
        }
    }

    return GameServerLocation{it->second.server_id, endpoint, it->second.room_id};
}

}  // namespace kfc
