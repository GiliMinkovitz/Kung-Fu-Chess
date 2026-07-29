#pragma once

#include "app/i_runtime_store.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace kfc {

class InMemoryRuntimeStore final : public IRuntimeStore {
public:
    InMemoryRuntimeStore() = default;

    [[nodiscard]] bool is_available() const override;
    void register_room(RoomId room_id, UserId white_player_id, UserId black_player_id,
                       std::string_view server_id, std::string_view endpoint) override;
    void unregister_room(RoomId room_id, UserId white_player_id, UserId black_player_id) override;
    void publish_server_heartbeat(std::string_view server_id, std::string_view region,
                                  const app::ServerMetrics& metrics) override;
    void deregister_server(std::string_view server_id) override;
    [[nodiscard]] std::optional<GameServerLocation> find_player_location(
        UserId user_id) const override;
    [[nodiscard]] std::vector<GameServerRecord> list_game_servers() const override;
    [[nodiscard]] std::optional<GameServerRecord> get_game_server(
        std::string_view server_id) const override;
    void set_matchmaking_queue(std::string_view region,
                               const std::vector<matchmaking::QueuedPlayer>& queue) override;
    [[nodiscard]] std::vector<matchmaking::QueuedPlayer> list_matchmaking_queue(
        std::string_view region) const override;

private:
    struct PlayerLocationEntry {
        RoomId room_id = 0;
        std::string server_id;
        std::string endpoint;
    };

    struct GameServerEntry {
        std::string endpoint;
        std::string allocation_endpoint;
        std::string region;
        std::size_t active_rooms = 0;
        std::string status;
        std::int64_t last_heartbeat = 0;
    };

    [[nodiscard]] static std::int64_t current_epoch_seconds();

    std::unordered_map<UserId, PlayerLocationEntry> player_locations_;
    std::unordered_map<std::string, GameServerEntry> game_servers_;
    std::unordered_map<std::string, std::vector<matchmaking::QueuedPlayer>> matchmaking_queues_;
};

}  // namespace kfc
