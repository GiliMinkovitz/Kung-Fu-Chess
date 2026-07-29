#pragma once

#include "app/game_server_location.h"
#include "app/game_server_record.h"
#include "app/server_metrics.h"
#include "matchmaking/queued_player.h"
#include "server/network/player_id.h"
#include "server/room/room_id.h"
#include "server/user/user_id.h"

#include <optional>
#include <string_view>
#include <vector>

namespace kfc {

class IRuntimeStore {
public:
    virtual ~IRuntimeStore() = default;

    [[nodiscard]] virtual bool is_available() const = 0;
    virtual void register_room(RoomId room_id, UserId white_player_id, UserId black_player_id,
                               std::string_view server_id, std::string_view endpoint) = 0;
    virtual void unregister_room(RoomId room_id, UserId white_player_id, UserId black_player_id) = 0;
    virtual void publish_server_heartbeat(std::string_view server_id, std::string_view region,
                                          const app::ServerMetrics& metrics) = 0;
    virtual void deregister_server(std::string_view server_id) = 0;
    [[nodiscard]] virtual std::optional<GameServerLocation> find_player_location(
        UserId user_id) const = 0;
    [[nodiscard]] virtual std::vector<GameServerRecord> list_game_servers() const = 0;
    [[nodiscard]] virtual std::optional<GameServerRecord> get_game_server(
        std::string_view server_id) const = 0;

    virtual void set_matchmaking_queue(std::string_view region,
                                       const std::vector<matchmaking::QueuedPlayer>& queue) = 0;
    [[nodiscard]] virtual std::vector<matchmaking::QueuedPlayer> list_matchmaking_queue(
        std::string_view region) const = 0;
};

}  // namespace kfc
