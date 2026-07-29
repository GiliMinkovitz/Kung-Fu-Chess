#include "app/in_memory_runtime_store.h"

#include <chrono>

namespace kfc {

bool InMemoryRuntimeStore::is_available() const {
    return false;
}

void InMemoryRuntimeStore::register_room(const RoomId room_id, const UserId white_player_id,
                                         const UserId black_player_id,
                                         const std::string_view server_id,
                                         const std::string_view endpoint) {
    const PlayerLocationEntry entry{room_id, std::string(server_id), std::string(endpoint)};
    player_locations_[white_player_id] = entry;
    player_locations_[black_player_id] = entry;
}

void InMemoryRuntimeStore::unregister_room(const RoomId, const UserId white_player_id,
                                           const UserId black_player_id) {
    player_locations_.erase(white_player_id);
    player_locations_.erase(black_player_id);
}

void InMemoryRuntimeStore::publish_server_heartbeat(const std::string_view server_id,
                                                    const std::string_view region,
                                                    const app::ServerMetrics& metrics) {
    GameServerEntry entry;
    entry.endpoint = metrics.endpoint;
    entry.allocation_endpoint = metrics.allocation_endpoint;
    entry.region = std::string(region);
    entry.active_rooms = metrics.active_rooms;
    entry.status = "healthy";
    entry.last_heartbeat = current_epoch_seconds();
    game_servers_[std::string(server_id)] = std::move(entry);
}

void InMemoryRuntimeStore::deregister_server(const std::string_view server_id) {
    game_servers_.erase(std::string(server_id));
}

std::optional<GameServerLocation> InMemoryRuntimeStore::find_player_location(
    const UserId user_id) const {
    const auto it = player_locations_.find(user_id);
    if (it == player_locations_.end()) {
        return std::nullopt;
    }

    return GameServerLocation{it->second.server_id, it->second.endpoint, it->second.room_id};
}

std::vector<GameServerRecord> InMemoryRuntimeStore::list_game_servers() const {
    std::vector<GameServerRecord> servers;
    servers.reserve(game_servers_.size());
    for (const auto& [server_id, entry] : game_servers_) {
        servers.push_back(GameServerRecord{server_id, entry.endpoint, entry.allocation_endpoint,
                                           entry.region, entry.active_rooms, entry.status,
                                           entry.last_heartbeat});
    }
    return servers;
}

std::optional<GameServerRecord> InMemoryRuntimeStore::get_game_server(
    const std::string_view server_id) const {
    const auto it = game_servers_.find(std::string(server_id));
    if (it == game_servers_.end()) {
        return std::nullopt;
    }

    return GameServerRecord{it->first, it->second.endpoint, it->second.allocation_endpoint,
                            it->second.region, it->second.active_rooms, it->second.status,
                            it->second.last_heartbeat};
}

std::int64_t InMemoryRuntimeStore::current_epoch_seconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

void InMemoryRuntimeStore::set_matchmaking_queue(
    const std::string_view region, const std::vector<matchmaking::QueuedPlayer>& queue) {
    matchmaking_queues_[std::string(region)] = queue;
}

std::vector<matchmaking::QueuedPlayer> InMemoryRuntimeStore::list_matchmaking_queue(
    const std::string_view region) const {
    const auto it = matchmaking_queues_.find(std::string(region));
    if (it == matchmaking_queues_.end()) {
        return {};
    }
    return it->second;
}

}  // namespace kfc
