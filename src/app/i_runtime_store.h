#pragma once

#include "app/game_server_location.h"
#include "app/server_metrics.h"
#include "server/room/room_id.h"
#include "server/user/user_id.h"

#include <optional>
#include <string_view>

namespace kfc {

class IRuntimeStore {
public:
    virtual ~IRuntimeStore() = default;

    [[nodiscard]] virtual bool is_available() const = 0;
    virtual void register_room(RoomId room_id, UserId white_player_id, UserId black_player_id,
                               std::string_view server_id) = 0;
    virtual void unregister_room(RoomId room_id, UserId white_player_id, UserId black_player_id) = 0;
    virtual void publish_server_heartbeat(std::string_view server_id, std::string_view region,
                                          const app::ServerMetrics& metrics) = 0;
    virtual void deregister_server(std::string_view server_id) = 0;
    [[nodiscard]] virtual std::optional<GameServerLocation> find_player_location(
        UserId user_id) const = 0;
};

}  // namespace kfc
