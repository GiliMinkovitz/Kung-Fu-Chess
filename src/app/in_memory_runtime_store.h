#pragma once

#include "app/i_runtime_store.h"

#include <string>
#include <unordered_map>

namespace kfc {

class InMemoryRuntimeStore final : public IRuntimeStore {
public:
    explicit InMemoryRuntimeStore(std::string game_server_endpoint = {});

    [[nodiscard]] bool is_available() const override;
    void register_room(RoomId room_id, UserId white_player_id, UserId black_player_id,
                       std::string_view server_id) override;
    void unregister_room(RoomId room_id, UserId white_player_id, UserId black_player_id) override;
    void publish_server_heartbeat(std::string_view server_id, std::string_view region,
                                  const app::ServerMetrics& metrics) override;
    void deregister_server(std::string_view server_id) override;
    [[nodiscard]] std::optional<GameServerLocation> find_player_location(
        UserId user_id) const override;

private:
    struct PlayerLocationEntry {
        RoomId room_id = 0;
        std::string server_id;
        std::string endpoint;
    };

    std::string game_server_endpoint_;
    std::unordered_map<UserId, PlayerLocationEntry> player_locations_;
    std::unordered_map<std::string, std::string> server_endpoints_;
};

}  // namespace kfc
